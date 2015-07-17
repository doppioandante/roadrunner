// == PREAMBLE ================================================

// * Licensed under the Apache License, Version 2.0; see README

/*
 * CudaGenerator.cpp
 *
 *  Created on: Aug 21, 2014
 *
 *  Author: JKM
 */

// == INCLUDES ================================================

# include "CudaGenerator.hpp"
# include "Hasher.hpp"
// # include "GitInfo.h" // Doesn't work

#include <fstream>
#include <sstream>

// == CODE ====================================================


namespace rr
{

namespace rrgpu
{

namespace dom
{

// class CudaGeneratorSBML {
// public:
//     /**
//      * @brief
//      * @details Given a component of the state vector,
//      * determine which side (if any) of the reaction
//      * it is on
//      */
//     int getReactionSide(const std::string& species_id);
// };

class StateContext {
public:
    virtual ~StateContext() {}

    void setf(const Variable* v) { f_ = v; }
    const Variable* getf() const { return f_; }

protected:
    /// Statevec
    const Variable* f_ = nullptr;
};

class RKStateContext :  public StateContext {
public:

    /// Empty ctor
    RKStateContext() {}

    /// Copy-construct with a different RK index
    RKStateContext(const RKStateContext& other, int rk)
      : RKStateContext(other) {
          rk_ = rk;
      }

    void setK(const Variable* v) { k = v; }
    const Variable* getK() const { return k; }

    int RKIndex() const { return getRKIndex(); }
    int getRKIndex() const { return rk_; }
    void setRKIndex(int rk) { rk_ = rk; }

    void setRKGen(const Variable* v) { rk_gen = v; }
    const Variable* getRKGen() const { return rk_gen; }
protected:
    /// RK coef
    const Variable* k = nullptr;
    /// Iteration of main int. loop
    const Variable* rk_gen = nullptr;
    /// The RK index
    int rk_ = 0;
};

class CudaGeneratorImpl {
public:
    typedef CudaGenerator::Precision Precision;

    /// Ctor
    CudaGeneratorImpl(GPUSimExecutableModel& mod)
      : mod_(mod) {
        if (Logger::getLevel() >= Logger::LOG_TRACE)
            enableDiagnostics();
    }

    void setPrecision(Precision p) {
        p_ = p;
    }

    Precision getPrecision() const {
        return p_;
    }

    ExpressionPtr generateReactionRateExp(const Reaction* r, StateContext* ctx);

    ExpressionPtr accumulate(ExpressionPtr&& sum, ExpressionPtr&& item, bool invert);

    ExpressionPtr generateEvalExp(int component, StateContext* ctx);

    ExpressionPtr getIntermediateValue(const FloatingSpecies* s, StateContext* ctx);

# if 0 && GPUSIM_MODEL_USE_SBML
    ExpressionPtr generateExpForSBMLASTNode(const Reaction* r, const libsbml::ASTNode* node, int rk_index);
// # else
// # error "Need SBML"
# endif

    ExpressionPtr generateExpForASTNode(const ModelASTNode* n, StateContext* ctx);

    /// Generate the model code
    void generate();

    /// Entry point into generated code
    GPUEntryPoint getEntryPoint();

    /// Return true if diagnostics should be emitted from the GPU code
    bool diagnosticsEnabled() const {
        return diag_;
    }

    /// Enable diagnostics dumped directly from the GPU
    void enableDiagnostics() {
        diag_ = true;
    }

    ExponentiationExpressionPtr generatePow(ExpressionPtr&& x, ExpressionPtr&& y) const {
        if (getPrecision() == Precision::Single)
            return mod.powSP(std::move(x), std::move(y));
        else
            return mod.pow(std::move(x), std::move(y));
    }

protected:

    ExpressionPtr getRKCoef(const Variable* coef, ExpressionPtr&& index, ExpressionPtr&& component) {
        return ExpressionPtr(new ArrayIndexExpression(coef,
                MacroExpression(RK_COEF_GET_OFFSET,
                std::move(index),
                std::move(component))));
    }

    ExpressionPtr getK(ExpressionPtr&& index, ExpressionPtr&& component, RKStateContext* ctx) {
        return getRKCoef(ctx->getK(), std::move(index), std::move(component));
    }

    /**
     * @brief Get the RK coef for a given index/component
     * @details Expression rvalue version
     */
    template <class IndexExpression, class ComponentExpression>
    ExpressionPtr getK(IndexExpression&& index, ComponentExpression&& component, RKStateContext* ctx) {
        return getK(
                ExpressionPtr(new IndexExpression(std::move(index))),
                ExpressionPtr(new ComponentExpression(std::move(component))),
          ctx);
    }

    ExpressionPtr getVecCoef(const Variable* vec, ExpressionPtr&& generation, ExpressionPtr&& component) {
        return ExpressionPtr(new ArrayIndexExpression(vec,
                MacroExpression(RK_STATE_VEC_GET_OFFSET,
                std::move(generation),
                std::move(component)
                  )));
    }

    ExpressionPtr getSV(ExpressionPtr&& generation, ExpressionPtr&& component, StateContext* ctx) {
        return getVecCoef(ctx->getf(), std::move(generation), std::move(component));
    }

    /**
     * @brief Get the RK coef for a given index/component
     * @details Expression rvalue version
     */
    template <class GenerationExpression, class ComponentExpression>
    ExpressionPtr getSV(GenerationExpression&& generation, ComponentExpression&& component, StateContext* ctx) {
        return getSV(
                ExpressionPtr(new GenerationExpression(std::move(generation))),
                ExpressionPtr(new ComponentExpression(std::move(component))),
          ctx);
    }

    // options
    Precision p_ = Precision::Single;


    GPUEntryPoint entry_;
//     Poco::SharedLibrary so_;
    CudaCodeCompiler compiler_;
    CudaExecutableModule exemodule_;
//     CudaGeneratorSBML sbmlgen_;
    GPUSimExecutableModel& mod_;
    CudaModule mod;

    // the RK coefficients
    Macro* RK_COEF_GET_OFFSET = nullptr;
    Macro* RK_STATE_VEC_GET_OFFSET = nullptr;

    CudaFunction* add_if_positive = nullptr;
    Variable* h = nullptr;
    Variable* f = nullptr;
    bool diag_ = false;

    bool statevec_is_shared_ = false;
};

CudaGenerator::CudaGenerator() {

}

CudaGenerator::~CudaGenerator() {

}

void CudaGenerator::generate(GPUSimExecutableModel& model) {
    impl_.reset(new CudaGeneratorImpl(model));
    Log(Logger::LOG_INFORMATION) << "Generated CUDA code will use fp precision: " << (getPrecision() == Precision::Single  ? "single" : "double");
    impl_->setPrecision(getPrecision());
    impl_->generate();
}

GPUEntryPoint CudaGenerator::getEntryPoint() {
    return impl_->getEntryPoint();
}

void CudaGenerator::setPrecision(Precision p) {
    p_ = p;
}

CudaGenerator::Precision CudaGenerator::getPrecision() const {
    return p_;
}

GPUEntryPoint CudaGeneratorImpl::getEntryPoint() {
    if (!entry_.bound())
        throw_gpusim_exception("No entry point set (did you forget to call generate first?)");
    return entry_;
}

ExpressionPtr CudaGeneratorImpl::getIntermediateValue(const FloatingSpecies* s, StateContext* ctxx) {
    assert(s && "Should not happen");
    int component = mod_.getStateVecComponent(s);

    assert(ctxx);
    auto ctx = dynamic_cast<RKStateContext*>(ctxx);
    assert(ctx);

    if (ctx->RKIndex() == 0)
        return getSV(VariableRefExpression(ctx->getRKGen()), LiteralIntExpression(component), ctxx);
    else if (ctx->RKIndex() == 1 || ctx->RKIndex() == 2)
        return ExpressionPtr(new SumExpression(getSV(VariableRefExpression(ctx->getRKGen()), LiteralIntExpression(component), ctxx),
                    ProductExpression(
                        ProductExpression(RealLiteralExpression(0.5), VariableRefExpression(h)),
                        getK(LiteralIntExpression(ctx->RKIndex()-1), LiteralIntExpression(component), ctx)
                      )));
    else if (ctx->RKIndex() == 3)
        return ExpressionPtr(new SumExpression(getSV(VariableRefExpression(ctx->getRKGen()), LiteralIntExpression(component), ctxx),
                    ProductExpression(
                        VariableRefExpression(h),
                        getK(LiteralIntExpression(ctx->RKIndex()-1), LiteralIntExpression(component), ctx)
                      )));
    else
        assert(0 && "Should not happen (0 <= ctx->RKIndex() < 4)");
}

ExpressionPtr CudaGeneratorImpl::generateExpForASTNode(const ModelASTNode* node, StateContext* ctx) {
    assert(node);
    // visitor is no good here (way too much overhead and not extensible enough)
    // sum
    if (auto n = dynamic_cast<const SumASTNode*>(node))
        return ExpressionPtr(new SumExpression(generateExpForASTNode(n->getLeft(), ctx), generateExpForASTNode(n->getRight(), ctx)));
    // difference
    else if (auto n = dynamic_cast<const DifferenceASTNode*>(node))
        return ExpressionPtr(new DifferenceExpression(generateExpForASTNode(n->getLeft(), ctx), generateExpForASTNode(n->getRight(), ctx)));
    // product
    else if (auto n = dynamic_cast<const ProductASTNode*>(node))
        return ExpressionPtr(new ProductExpression(generateExpForASTNode(n->getLeft(), ctx), generateExpForASTNode(n->getRight(), ctx)));
    // division
    else if (auto n = dynamic_cast<const QuotientASTNode*>(node))
        return ExpressionPtr(new QuotientExpression(generateExpForASTNode(n->getLeft(), ctx), generateExpForASTNode(n->getRight(), ctx)));
    // exponentiation
    else if (auto n = dynamic_cast<const ExponentiationASTNode*>(node))
        return ExpressionPtr(generatePow(generateExpForASTNode(n->getLeft(), ctx), generateExpForASTNode(n->getRight(), ctx)));
    // floating species ref
    else if (auto n = dynamic_cast<const FloatingSpeciesRefASTNode*>(node))
        return getIntermediateValue(n->getFloatingSpecies(), ctx);
    // parameter ref
    else if (auto n = dynamic_cast<const ParameterRefASTNode*>(node))
        return ExpressionPtr(new RealLiteralExpression(n->getParameterVal()));
    // integer
    else if (auto n = dynamic_cast<const IntegerLiteralASTNode*>(node))
        return ExpressionPtr(new LiteralIntExpression(n->getValue()));
    else
        return ExpressionPtr(new LiteralIntExpression(12345));
}

# if 0 && GPUSIM_MODEL_USE_SBML
ExpressionPtr CudaGeneratorImpl::generateExpForSBMLASTNode(const Reaction* r, const libsbml::ASTNode* node, int rk_index) {
    assert(r && node);
    switch (node->getType()) {
        case libsbml::AST_TIMES:
            return ExpressionPtr(new ProductExpression(generateExpForSBMLASTNode(r, node->getChild(0), rk_index), generateExpForSBMLASTNode(r, node->getChild(1), rk_index)));
        case libsbml::AST_NAME:
            if (r->isParameter(node->getName()))
                return ExpressionPtr(new RealLiteralExpression(r->getParameterVal(node->getName())));
            if (r->isParticipant(node->getName()))
                return getIntermediateValue(r->getFloatingSpecies(node->getName()), rk_index);
            else
                return ExpressionPtr(new LiteralIntExpression(12345));
        default:
            return ExpressionPtr(new LiteralIntExpression(0));
    }
}
// # else
// # error "Need SBML"
# endif

ExpressionPtr CudaGeneratorImpl::generateReactionRateExp(const Reaction* r, StateContext* ctx) {
    if (r->isReversible())
        return generateExpForASTNode(r->getKineticLaw().getAlgebra().getRoot(), ctx);
    else
        return ExpressionPtr(new FunctionCallExpression(add_if_positive, generateExpForASTNode(r->getKineticLaw().getAlgebra().getRoot(), ctx)));
}

ExpressionPtr CudaGeneratorImpl::accumulate(ExpressionPtr&& sum, ExpressionPtr&& item, bool invert) {
    if (sum)
        return invert ?
            ExpressionPtr(new DifferenceExpression(std::move(sum), std::move(item))) :
            ExpressionPtr(new SumExpression(std::move(sum), std::move(item)));
    else
        return invert ?
            ExpressionPtr(new UnaryMinusExpression(std::move(item))) :
            ExpressionPtr(std::move(item));
}

ExpressionPtr CudaGeneratorImpl::generateEvalExp(int component, StateContext* ctx) {
    // get the species corresponding to this component in the state vec
    const FloatingSpecies* s = mod_.getFloatingSpeciesFromSVComponent(component);

    // accumulate reaction rates in this expression
    ExpressionPtr sum;

    for (const Reaction* r : mod_.getReactions())
        if (int factor = mod_.getReactionSideFac(r, s)) {
            assert((factor == 1 || factor == -1) && "Should not happen");
            sum = accumulate(std::move(sum), generateReactionRateExp(r, ctx), factor==-1);
        }

    return std::move(sum);
}

void CudaGeneratorImpl::generate() {
    auto generate_start = std::chrono::high_resolution_clock::now();

    std::string entryName = "cuEntryPoint";

    // dump the state vector assignments
    mod_.dumpStateVecAssignments();

    // get the size of the state vector
    int n = mod_.getStateVector(NULL);

    std::unique_ptr<RKStateContext> ctx(new RKStateContext());

    // construct the DOM

    // macros
    Macro* N = mod.addMacro(Macro("N", std::to_string(n)));

    Macro* RK4ORDER = mod.addMacro(Macro("RK4ORDER", "4"));

    Macro* RK_COEF_LEN = mod.addMacro(Macro("RK_COEF_LEN", "(RK4ORDER*N)"));
    RK_COEF_GET_OFFSET = mod.addMacro(Macro("RK_COEF_GET_OFFSET", "((idx)*N + (component))","idx", "component"));

    Macro* RK_GET_COMPONENT = mod.addMacro(Macro("RK_GET_COMPONENT", "(threadIdx.x/32)"));

    Macro* RK_STATE_VEC_LEN = mod.addMacro(Macro("RK_STATE_VEC_LEN", "(km*N)"));
    RK_STATE_VEC_GET_OFFSET = mod.addMacro(Macro("RK_STATE_VEC_GET_OFFSET", "((gen)*N + (component))", "gen", "component"));

    Macro* RK_TIME_VEC_LEN = mod.addMacro(Macro("RK_TIME_VEC_LEN", "km"));

    // typedef for float or double
    Type* RKReal;
    if (p_ ==  Precision::Single)
        RKReal = TypedefStatement::downcast(mod.addStatement(StatementPtr(new TypedefStatement(BaseTypes::getTp(BaseTypes::FLOAT), "RKReal"))))->getAlias();
    else
        RKReal = TypedefStatement::downcast(mod.addStatement(StatementPtr(new TypedefStatement(BaseTypes::getTp(BaseTypes::DOUBLE), "RKReal"))))->getAlias();

    Type* pRKReal = BaseTypes::get().addPointer(RKReal);

    Type* pRKReal_volatile = BaseTypes::get().addVolatile(pRKReal);

    // add_if_positive

    add_if_positive = mod.addFunction(CudaFunction("add_if_positive", RKReal, {FunctionParameter(RKReal, "x")}));

    {
        add_if_positive->setIsDeviceFun(true);

        add_if_positive->addStatement(StatementPtr(new ReturnStatement(
            TernarySwitch(
                LTComparisonExpression(RealLiteralExpression(0.),  VariableRefExpression(add_if_positive->getPositionalParam(0))),
                VariableRefExpression(add_if_positive->getPositionalParam(0)),
                RealLiteralExpression(0.)
              ))));
    }

    CudaFunction* PrintCoefs = mod.addFunction(CudaFunction("PrintCoefs", BaseTypes::getTp(BaseTypes::VOID), {FunctionParameter(pRKReal, "k")}));

    // generate PrintCoefs
    {
        PrintCoefs->setIsDeviceFun(true);

        PrintCoefs->addStatement(ExpressionPtr(new FunctionCallExpression(mod.getCudaSyncThreads())));

        IfStatement* if_thd1 = IfStatement::downcast(PrintCoefs->addStatement(StatementPtr(new IfStatement(
                    ExpressionPtr(new EqualityCompExpression(mod.getThreadIdx(CudaModule::IndexComponent::x), LiteralIntExpression(0)))
                ))));

        // printf
        if_thd1->getBody().addStatement(ExpressionPtr(new FunctionCallExpression(mod.getPrintf(), ExpressionPtr(new StringLiteralExpression("PrintCoefs\\n")))));

        std::string printcoef_fmt_str;
        for (int rk_index=0; rk_index<4; ++rk_index) {
            for (int component=0; component<n; ++component) {
                ExpressionStatement::insert(if_thd1->getBody(), FunctionCallExpression(
                        mod.getPrintf(),
                        StringLiteralExpression("%3.3f "),
                        getRKCoef(PrintCoefs->getPositionalParam(0), ExpressionPtr(new LiteralIntExpression(rk_index)), ExpressionPtr(new LiteralIntExpression(component)))
                    ));
            }
            ExpressionStatement::insert(if_thd1->getBody(), FunctionCallExpression(
                mod.getPrintf(),
                StringLiteralExpression("\\n")
                ));
        }

        PrintCoefs->addStatement(ExpressionPtr(new FunctionCallExpression(mod.getCudaSyncThreads())));
    }

    CudaFunction* PrintStatevec = mod.addFunction(CudaFunction("PrintStatevec", BaseTypes::getTp(BaseTypes::VOID), {FunctionParameter(pRKReal, "f"), FunctionParameter(BaseTypes::getTp(BaseTypes::INT), "generation")}));

    {
        PrintStatevec->setIsDeviceFun(true);

        PrintStatevec->addStatement(ExpressionPtr(new FunctionCallExpression(mod.getCudaSyncThreads())));

        IfStatement* if_thd1 = IfStatement::downcast(PrintStatevec->addStatement(StatementPtr(new IfStatement(
            ExpressionPtr(new EqualityCompExpression(mod.getThreadIdx(CudaModule::IndexComponent::x), LiteralIntExpression(0)))
            ))));

        std::string sv_fmt_str = "statevec ";
        for (int component=0; component<n; ++component)
            sv_fmt_str += "%f ";
        sv_fmt_str += "\\n";

        // print the state vector
        FunctionCallExpression* printf_statevec = FunctionCallExpression::downcast(ExpressionStatement::insert(if_thd1->getBody(), FunctionCallExpression(
                mod.getPrintf(),
                StringLiteralExpression(sv_fmt_str)
            ))->getExpression());

        for (int component=0; component<n; ++component)
            printf_statevec->passArgument(ExpressionPtr(
                new ArrayIndexExpression(PrintStatevec->getPositionalParam(0),
                    MacroExpression(RK_STATE_VEC_GET_OFFSET,
                    VariableRefExpression(PrintStatevec->getPositionalParam(1)),
                    LiteralIntExpression(component)))
            ));

        PrintStatevec->addStatement(ExpressionPtr(new FunctionCallExpression(mod.getCudaSyncThreads())));
    }

    // generate PrintRxnRates
    CudaFunction* PrintRxnRates = mod.addFunction(CudaFunction("PrintRxnRates", BaseTypes::getTp(BaseTypes::VOID), {FunctionParameter(pRKReal, "f"), FunctionParameter(BaseTypes::getTp(BaseTypes::INT), "j")}));

    {
        PrintRxnRates->setIsDeviceFun(true);

        PrintRxnRates->addStatement(ExpressionPtr(new FunctionCallExpression(mod.getCudaSyncThreads())));

        IfStatement* if_thd1 = IfStatement::downcast(PrintRxnRates->addStatement(StatementPtr(new IfStatement(
                    ExpressionPtr(new EqualityCompExpression(mod.getThreadIdx(CudaModule::IndexComponent::x), LiteralIntExpression(0)))
                ))));

        // printf
        if_thd1->getBody().addStatement(ExpressionPtr(new FunctionCallExpression(mod.getPrintf(), ExpressionPtr(new StringLiteralExpression("PrintRxnRates\\n")))));

        RKStateContext newctx(*ctx);
        newctx.setf(PrintRxnRates->getPositionalParam(0));
        newctx.setRKGen(PrintRxnRates->getPositionalParam(1));
        for (const Reaction* r : mod_.getReactions()) {
            // loop through rxns
            ExpressionStatement::insert(if_thd1->getBody(), FunctionCallExpression(
                    mod.getPrintf(),
                    StringLiteralExpression(r->getId() + ": %3.3f "),
                    generateReactionRateExp(r, &newctx)
                ));
            ExpressionStatement::insert(if_thd1->getBody(), FunctionCallExpression(
                mod.getPrintf(),
                StringLiteralExpression("\\n")
                ));
        }

        PrintRxnRates->addStatement(ExpressionPtr(new FunctionCallExpression(mod.getCudaSyncThreads())));
    }

    CudaKernel* kernel = CudaKernel::downcast(mod.addFunction(CudaKernel("GPUIntMEBlockedRK4", BaseTypes::getTp(BaseTypes::VOID), {FunctionParameter(BaseTypes::getTp(BaseTypes::INT), "km"), FunctionParameter(pRKReal, "kt"), FunctionParameter(pRKReal, "kv")})));

    // the step size
    assert(kernel->getNumPositionalParams() == 3 && "wrong num of params - signature of kernel changed?");
    assert(kernel->getPositionalParam(0)->getType()->isIdentical(BaseTypes::getTp(BaseTypes::INT)) && "param has wrong type - signature of kernel changed? ");
    assert(kernel->getPositionalParam(1)->getType()->isIdentical(pRKReal) && "param has wrong type - signature of kernel changed? ");
    assert(kernel->getPositionalParam(2)->getType()->isIdentical(pRKReal) && "param has wrong type - signature of kernel changed? ");

    // generate the kernel
    {
        const FunctionParameter* km = kernel->getPositionalParam(0);
        const FunctionParameter* kt = kernel->getPositionalParam(1);
        const FunctionParameter* kv = kernel->getPositionalParam(2);

        // declare shared memory
        Variable* shared_buf = CudaVariableDeclarationExpression::downcast(ExpressionStatement::insert(*kernel, CudaVariableDeclarationExpression(kernel->addVariable(Variable(BaseTypes::get().addArray(RKReal), "shared_buf")), true))->getExpression())->getVariable();

        ctx->setK(VariableInitExpression::downcast(ExpressionStatement::insert(*kernel, VariableInitExpression(kernel->addVariable(Variable(pRKReal, "k")), ReferenceExpression(ArrayIndexExpression(shared_buf, LiteralIntExpression(0)))))->getExpression())->getVariable());

        if (statevec_is_shared_)
            f = VariableInitExpression::downcast(ExpressionStatement::insert(*kernel, VariableInitExpression(kernel->addVariable(Variable(pRKReal, "f")), ReferenceExpression(ArrayIndexExpression(ctx->getK(), MacroExpression(RK_COEF_LEN)))))->getExpression())->getVariable();
        else
            f = VariableInitExpression::downcast(ExpressionStatement::insert(*kernel, VariableInitExpression(kernel->addVariable(Variable(pRKReal, "f")), VariableRefExpression(kv)))->getExpression())->getVariable();

        ctx->setf(f);

        Variable* t = nullptr;
        if (statevec_is_shared_)
            t = VariableInitExpression::downcast(ExpressionStatement::insert(*kernel, VariableInitExpression(kernel->addVariable(Variable(pRKReal, "t")), ReferenceExpression(ArrayIndexExpression(f, MacroExpression(RK_STATE_VEC_LEN)))))->getExpression())->getVariable();
        else
            t = VariableInitExpression::downcast(ExpressionStatement::insert(*kernel, VariableInitExpression(kernel->addVariable(Variable(pRKReal, "t")), VariableRefExpression(kt)))->getExpression())->getVariable();

//         if (diagnosticsEnabled()) {
//             // printf
//             kernel->addStatement(ExpressionPtr(new FunctionCallExpression(mod.getPrintf(), ExpressionPtr(new StringLiteralExpression("in kernel\\n")))));
//         }

        {
            // initialize k (the RK coefficients)

            for (int i=0; i<4; ++i) {
                // init expression for k
                AssignmentExpression* k_init_assn =
                    AssignmentExpression::downcast(ExpressionStatement::insert(*kernel, AssignmentExpression(
                    getK(LiteralIntExpression(i), MacroExpression(RK_GET_COMPONENT), ctx.get()),
                    LiteralIntExpression(0)))->getExpression());

//                 if (diagnosticsEnabled()) {
//                     // printf showing the init'd value of k
//                     ExpressionStatement::insert(*kernel, FunctionCallExpression(
//                         mod.getPrintf(),
//                         StringLiteralExpression("k[RK_COEF_GET_OFFSET(%d, %d)] = %f\\n"),
//                         LiteralIntExpression(i),
//                         MacroExpression(RK_GET_COMPONENT),
//                         k_init_assn->getLHS()->clone()
//                     ));
//                 }
            }


            // sync & print coefs

            IfStatement* if_thd1 = IfStatement::downcast(kernel->addStatement(StatementPtr(new IfStatement(
                    ExpressionPtr(new EqualityCompExpression(mod.getThreadIdx(CudaModule::IndexComponent::x), LiteralIntExpression(0)))
                ))));
            // printf
            if_thd1->getBody().addStatement(ExpressionPtr(new FunctionCallExpression(mod.getPrintf(), ExpressionPtr(new StringLiteralExpression("**** Initial rk coefs ****\\n")))));

            kernel->addStatement(ExpressionPtr(new FunctionCallExpression(mod.getCudaSyncThreads())));
            kernel->addStatement(ExpressionPtr(new FunctionCallExpression(PrintCoefs, VariableRefExpression(ctx->getK()))));

//             for (int i=0; i<4; ++i) {
//                 if (diagnosticsEnabled()) {
//                     // printf showing the init'd value of k
//                     ExpressionStatement::insert(*kernel, FunctionCallExpression(
//                         mod.getPrintf(),
//                         StringLiteralExpression("k[RK_COEF_GET_OFFSET(%d, %d)] = %f\\n"),
//                         LiteralIntExpression(i),
//                         MacroExpression(RK_GET_COMPONENT),
//                         getK(LiteralIntExpression(i), MacroExpression(RK_GET_COMPONENT))
//                     ));
//                 }
//             }
        }

        {
            SwitchStatement* component_switch = SwitchStatement::insert(*kernel, MacroExpression(RK_GET_COMPONENT));

            for (int component=0; component<n; ++component) {
                component_switch->addCase(LiteralIntExpression(component));

                // initialize f (the state vector)
                AssignmentExpression* f_init_assn =
                    AssignmentExpression::downcast(ExpressionStatement::insert(component_switch->getBody(),
                    AssignmentExpression(
                        ArrayIndexExpression(f,
                MacroExpression(RK_STATE_VEC_GET_OFFSET,
                LiteralIntExpression(0),
                LiteralIntExpression(component))),
                    RealLiteralExpression(mod_.getFloatingSpeciesFromSVComponent(component)->getInitialConcentration())))->getExpression());

//                 if (diagnosticsEnabled()) {
//                     ExpressionStatement::insert(component_switch->getBody(), FunctionCallExpression(
//                         mod.getPrintf(),
//                         StringLiteralExpression("f[RK_STATE_VEC_GET_OFFSET(%d, %d)] = %f\\n"),
//                         LiteralIntExpression(0),
//                         LiteralIntExpression(component),
//                         f_init_assn->getLHS()->clone()
//                     ));
//                 }

                component_switch->addBreak();
            }

            IfStatement* if_thd1 = IfStatement::downcast(kernel->addStatement(StatementPtr(new IfStatement(
                    ExpressionPtr(new EqualityCompExpression(mod.getThreadIdx(CudaModule::IndexComponent::x), LiteralIntExpression(0)))
                ))));
            if_thd1->getBody().addStatement(ExpressionPtr(new FunctionCallExpression(mod.getPrintf(), ExpressionPtr(new StringLiteralExpression("**** Initial state vec ****\\n")))));

            kernel->addStatement(ExpressionPtr(new FunctionCallExpression(PrintStatevec, VariableRefExpression(f), LiteralIntExpression(0))));
        }

        // initialize the time values
        kernel->addStatement(ExpressionPtr(
            new FunctionCallExpression(
                mod.getRegMemcpy(),
                VariableRefExpression(t),
                VariableRefExpression(kt),
                ProductExpression(
                    VariableRefExpression(km), FunctionCallExpression(mod.getSizeof(), TypeRefExpression(RKReal))
                  )
              )));

        kernel->addStatement(ExpressionPtr(new FunctionCallExpression(mod.getCudaSyncThreads())));

        // print the initial coefficients
        if (diagnosticsEnabled()) {
            // print coefs
            // BUG? order reversed?
            kernel->addStatement(ExpressionPtr(new FunctionCallExpression(PrintCoefs, VariableRefExpression(ctx->getK()))));
            kernel->addStatement(ExpressionPtr(new FunctionCallExpression(mod.getCudaSyncThreads())));
        }

        // print the initial reaction rates
        if (diagnosticsEnabled()) {
            IfStatement* if_thd1 = IfStatement::downcast(kernel->addStatement(StatementPtr(new IfStatement(
                    ExpressionPtr(new EqualityCompExpression(mod.getThreadIdx(CudaModule::IndexComponent::x), LiteralIntExpression(0)))
                ))));
            if_thd1->getBody().addStatement(ExpressionPtr(new FunctionCallExpression(mod.getPrintf(), ExpressionPtr(new StringLiteralExpression("**** Initial reaction rates ****\\n")))));

            if_thd1->getBody().addStatement(ExpressionPtr(new FunctionCallExpression(mod.getCudaSyncThreads())));
            if_thd1->getBody().addStatement(ExpressionPtr(new FunctionCallExpression(PrintRxnRates, VariableRefExpression(ctx->getf()), LiteralIntExpression(0))));
            if_thd1->getBody().addStatement(ExpressionPtr(new FunctionCallExpression(mod.getCudaSyncThreads())));
        }

        // main integration loop
        {
            ForStatement* update_coef_loop = ForStatement::downcast(kernel->addStatement(ForStatement::make()));

            Variable* rk_gen = update_coef_loop->addVariable(Variable(BaseTypes::getTp(BaseTypes::INT), "j"));

            ctx->setRKGen(rk_gen);

            update_coef_loop->setInitExp(ExpressionPtr(new VariableInitExpression(rk_gen, ExpressionPtr(new LiteralIntExpression(0)))));

            update_coef_loop->setCondExp(ExpressionPtr(new LTComparisonExpression(ExpressionPtr(new VariableRefExpression(rk_gen)), ExpressionPtr(
                new DifferenceExpression(VariableRefExpression(km), LiteralIntExpression(1))
              ))));

            update_coef_loop->setLoopExp(ExpressionPtr(new PreincrementExpression(ExpressionPtr(new VariableRefExpression(rk_gen)))));

            // get step size
            h = update_coef_loop->addVariable(Variable(RKReal, "h"));
            VariableInitExpression::downcast(ExpressionStatement::insert(update_coef_loop->getBody(),
            VariableInitExpression(h,
              DifferenceExpression(
                  ArrayIndexExpression(t, SumExpression(VariableRefExpression(rk_gen), LiteralIntExpression(1))),
                  ArrayIndexExpression(t, VariableRefExpression(rk_gen))
              )
              ))->getExpression());

              if (diagnosticsEnabled()) {
                    // printf
//                     update_coef_loop->getBody().addStatement(ExpressionPtr(new FunctionCallExpression(mod.getCudaSyncThreads())));
//                     update_coef_loop->getBody().addStatement(ExpressionPtr(new FunctionCallExpression(mod.getPrintf(), ExpressionPtr(new StringLiteralExpression("loop\\n")))));
//                     update_coef_loop->getBody().addStatement(ExpressionPtr(new FunctionCallExpression(mod.getCudaSyncThreads())));
            }

            for (int rk_index=0; rk_index<4; ++rk_index) {
                SwitchStatement* component_switch = SwitchStatement::insert(update_coef_loop->getBody(), MacroExpression(RK_GET_COMPONENT));

                for (int component=0; component<n; ++component) {
                    component_switch->addCase(LiteralIntExpression(component));

                RKStateContext newctx(*ctx, rk_index);
                // RK coef computation
                AssignmentExpression* k_assn =
                    AssignmentExpression::downcast(ExpressionStatement::insert(component_switch->getBody(), AssignmentExpression(
                    ExpressionPtr(new ArrayIndexExpression(ctx->getK(),
                    MacroExpression(RK_COEF_GET_OFFSET,
                    LiteralIntExpression(newctx.getRKIndex()),
                    LiteralIntExpression(component)))),
                    generateEvalExp(component, &newctx)))->getExpression());

                    component_switch->addBreak();
                }

                update_coef_loop->getBody().addStatement(ExpressionPtr(new FunctionCallExpression(mod.getCudaSyncThreads())));
            }

            // update the state vector
            ExpressionStatement::insert(update_coef_loop->getBody(), AssignmentExpression(
                        ExpressionPtr(new ArrayIndexExpression(f,
                        MacroExpression(RK_STATE_VEC_GET_OFFSET,
                        SumExpression(VariableRefExpression(rk_gen), LiteralIntExpression(1)),
                        MacroExpression(RK_GET_COMPONENT)))),
                        SumExpression(
                        // prev value
                        ArrayIndexExpression(f,
                        MacroExpression(RK_STATE_VEC_GET_OFFSET,
                        VariableRefExpression(rk_gen),
                        MacroExpression(RK_GET_COMPONENT))),
                        // RK approx. for increment
                        ProductExpression(
                        ProductExpression(RealLiteralExpression(0.166666667), VariableRefExpression(h)),
                        SumExpression(SumExpression(SumExpression(
                            getK(LiteralIntExpression(0), MacroExpression(RK_GET_COMPONENT), ctx.get()),
                            ProductExpression(RealLiteralExpression(2.), getK(LiteralIntExpression(1), MacroExpression(RK_GET_COMPONENT), ctx.get()))),
                            ProductExpression(RealLiteralExpression(2.), getK(LiteralIntExpression(2), MacroExpression(RK_GET_COMPONENT), ctx.get()))),
                            getK(LiteralIntExpression(3), MacroExpression(RK_GET_COMPONENT), ctx.get())
                          )))
              ));

            update_coef_loop->getBody().addStatement(ExpressionPtr(new FunctionCallExpression(mod.getCudaSyncThreads())));

            if (diagnosticsEnabled()) {
                update_coef_loop->getBody().addStatement(ExpressionPtr(new FunctionCallExpression(PrintStatevec, VariableRefExpression(f), SumExpression(VariableRefExpression(rk_gen), LiteralIntExpression(1)))));

//                     ExpressionStatement::insert(if_thd1->getBody(), FunctionCallExpression(
//                             mod.getPrintf(),
//                             StringLiteralExpression("h = %f\\n"),
//                             VariableRefExpression(h)
//                         ));
                update_coef_loop->getBody().addStatement(ExpressionPtr(new FunctionCallExpression(mod.getCudaSyncThreads())));
            }
            if (diagnosticsEnabled()) {
                // print coefs
                update_coef_loop->getBody().addStatement(ExpressionPtr(new FunctionCallExpression(mod.getCudaSyncThreads())));
                update_coef_loop->getBody().addStatement(ExpressionPtr(new FunctionCallExpression(PrintCoefs, VariableRefExpression(ctx->getK()))));
            }
        }


        if (statevec_is_shared_)
        // copy results to passed parameters
            kernel->addStatement(ExpressionPtr(
                new FunctionCallExpression(
                    mod.getRegMemcpy(),
                    VariableRefExpression(kv),
                    VariableRefExpression(f),
                    ProductExpression(ProductExpression(VariableRefExpression(km), MacroExpression(N)), FunctionCallExpression(mod.getSizeof(), TypeRefExpression(RKReal)))
                )));
    }

    CudaFunction* entry = mod.addFunction(CudaFunction(entryName, BaseTypes::getTp(BaseTypes::VOID), {FunctionParameter(BaseTypes::getTp(BaseTypes::INT), "km"), FunctionParameter(pRKReal, "kt"), FunctionParameter(pRKReal, "kv")}));

    // generate the entry
    {
        const FunctionParameter* km = entry->getPositionalParam(0);
        const FunctionParameter* kt = entry->getPositionalParam(1);
        const FunctionParameter* kv = entry->getPositionalParam(2);

        if (diagnosticsEnabled()) {
            // printf
            entry->addStatement(ExpressionPtr(new FunctionCallExpression(mod.getPrintf(), ExpressionPtr(new StringLiteralExpression("in cuda\\n")))));
        }

        // allocate mem for tvalues
        Variable* tvals =
            VariableDeclarationExpression::downcast(ExpressionStatement::insert(entry,
            VariableDeclarationExpression(entry->addVariable(Variable(pRKReal, "tvals"))))->getExpression())->getVariable();
        entry->addStatement(ExpressionPtr(new FunctionCallExpression(mod.getCudaMalloc(),
        ReferenceExpression(VariableRefExpression(tvals)),
        ProductExpression(VariableRefExpression(km), FunctionCallExpression(mod.getSizeof(), TypeRefExpression(RKReal))))));

        // copy tvalues to device memory
        entry->addStatement(ExpressionPtr(
            new FunctionCallExpression(
                mod.getCudaMemcpy(),
                VariableRefExpression(tvals),
                VariableRefExpression(kt),
                ProductExpression(VariableRefExpression(km), FunctionCallExpression(mod.getSizeof(), TypeRefExpression(RKReal))),
                SymbolExpression("cudaMemcpyHostToDevice")
              )));

        // allocate mem for results
        Variable* results =
            VariableDeclarationExpression::downcast(ExpressionStatement::insert(entry,
            VariableDeclarationExpression(entry->addVariable(Variable(pRKReal, "results"))))->getExpression())->getVariable();
        entry->addStatement(ExpressionPtr(new FunctionCallExpression(mod.getCudaMalloc(),
        ReferenceExpression(VariableRefExpression(results)),
        ProductExpression(ProductExpression(VariableRefExpression(km), MacroExpression(N)), FunctionCallExpression(mod.getSizeof(), TypeRefExpression(RKReal))))));

        // call the kernel
        CudaKernelCallExpression* kernel_call = CudaKernelCallExpression::downcast(ExpressionStatement::insert(entry,
        CudaKernelCallExpression(1, 1, 1,
        kernel,
        VariableRefExpression(km),
        VariableRefExpression(tvals),
        VariableRefExpression(results)
          ))->getExpression());

        kernel_call->setNumThreads(ProductExpression(MacroExpression(N), LiteralIntExpression(32)));

        if (statevec_is_shared_)
            kernel_call->setSharedMemSize(SumExpression(SumExpression(
                ProductExpression(MacroExpression(RK_COEF_LEN), FunctionCallExpression(mod.getSizeof(), TypeRefExpression(RKReal))), // Coefficient size
                ProductExpression(MacroExpression(RK_STATE_VEC_LEN), FunctionCallExpression(mod.getSizeof(), TypeRefExpression(RKReal)))), // State vector size
                ProductExpression(MacroExpression(RK_TIME_VEC_LEN), FunctionCallExpression(mod.getSizeof(), TypeRefExpression(RKReal))) // Time vector size
                ));
        else
            kernel_call->setSharedMemSize(
                ProductExpression(MacroExpression(RK_COEF_LEN), FunctionCallExpression(mod.getSizeof(), TypeRefExpression(RKReal))) // Coefficient size
                );

        // copy device results to passed parameters
        entry->addStatement(ExpressionPtr(
            new FunctionCallExpression(
                mod.getCudaMemcpy(),
                VariableRefExpression(kv),
                VariableRefExpression(results),
                ProductExpression(ProductExpression(VariableRefExpression(km), MacroExpression(N)), FunctionCallExpression(mod.getSizeof(), TypeRefExpression(RKReal))),
                SymbolExpression("cudaMemcpyDeviceToHost")
            )));

        // free the result mem
        entry->addStatement(ExpressionPtr(new FunctionCallExpression(mod.getCudaFree(), VariableRefExpression(results))));

        // call cudaDeviceSynchronize
        entry->addStatement(ExpressionPtr(new FunctionCallExpression(mod.getCudaDeviceSynchronize())));
    }

    auto generate_finish = std::chrono::high_resolution_clock::now();

    Log(Logger::LOG_INFORMATION) << "Generating model DOM took " << std::chrono::duration_cast<std::chrono::milliseconds>(generate_finish - generate_start).count() << " ms";

    // generate temporary file names

//     Log(Logger::LOG_DEBUG) << "Last git commit: " << getGitLastCommit();

    // The git version info definitely doesn't work as of 4ad273339b84e3149196b5a08936375e42edc13b
    // (reports last commit 19abcd7d7de27be773c640bfdde6455c9bcf4125)
    std::string hashedid = mod_.getSBMLHash().str();

//     Log(Logger::LOG_DEBUG) << "Hashed model id: " << hashedid;

     exemodule_ = compiler_.generate(mod, CudaCodeCompiler::GenerateParams(p_, hashedid, entryName));
     entry_ = exemodule_.getEntry();

    // ensure that the function has the correct signature
    if (getPrecision() ==  Precision::Single)
        assert(sizeof(float) == RKReal->Sizeof());
    else if(getPrecision() ==  Precision::Double)
        assert(sizeof(double) == RKReal->Sizeof());
    else
        assert(0 && "Unknown precision");

}

} // namespace dom

} // namespace rrgpu

} // namespace rr