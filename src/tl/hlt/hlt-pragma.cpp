/*--------------------------------------------------------------------
  (C) Copyright 2006-2011 Barcelona Supercomputing Center 
                          Centro Nacional de Supercomputacion
  
  This file is part of Mercurium C/C++ source-to-source compiler.
  
  See AUTHORS file in the top level directory for information 
  regarding developers and contributors.
  
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 3 of the License, or (at your option) any later version.
  
  Mercurium C/C++ source-to-source compiler is distributed in the hope
  that it will be useful, but WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the GNU Lesser General Public License for more
  details.
  
  You should have received a copy of the GNU Lesser General Public
  License along with Mercurium C/C++ source-to-source compiler; if
  not, write to the Free Software Foundation, Inc., 675 Mass Ave,
  Cambridge, MA 02139, USA.
--------------------------------------------------------------------*/



#include "hlt-pragma.hpp"
#include "hlt-unroll.hpp"
#include "hlt-blocking.hpp"
#include "hlt-distribution.hpp"
#include "hlt-fusion.hpp"
#include "hlt-interchange.hpp"
#include "hlt-collapse.hpp"
#include "hlt-composition.hpp"
#include "hlt-outline.hpp"
#include "hlt-extension.hpp"
#include "hlt-peeling.hpp"
#include "hlt-task-aggregation.hpp"
#include "hlt-stripmine.hpp"
#include "hlt-exception.hpp"
#include "hlt-simd.hpp"
#include "tl-simd.hpp"


#include <algorithm>

using namespace TL::HLT;
using namespace TL::SIMD;

static bool _allow_identity = true;
static std::string _allow_identity_str;


static TL::ObjectList<TL::Symbol> builtin_vr_list;
static TL::ObjectList<TL::Symbol> builtin_ve_list;
static TL::ObjectList<TL::Symbol> builtin_gf_list;
static TL::ObjectList<TL::Symbol> builtin_vc_list;

static void update_identity_flag(const std::string &str)
{
    TL::parse_boolean_option("disable_identity",
            str,
            _allow_identity,
            "Option 'disable_identity' is a boolean flag");
}

//__builtin_vector_reference overload
static scope_entry_t* solve_vector_ref_overload_name(scope_entry_t* overloaded_function, 
        type_t** types,  
        AST *arguments UNUSED_PARAMETER,
        int num_arguments)
{
    char name[256];
    int i;
    char found_match = 0;
    scope_entry_t* result = NULL;

    if (num_arguments != 1)
    {
        internal_error("hlt-simd builtin '%s' only allows one parameter\n", 
                        overloaded_function->symbol_name);
    }

    for(i=1; i<builtin_vr_list.size(); i++) 
    {
        if (equivalent_types(get_unqualified_type(types[0]),
                    function_type_get_parameter_type_num(builtin_vr_list[i].get_type()
                        .get_internal_type(), 0)))
        {
            return builtin_vr_list[i].get_internal_symbol();
        }
    }

    //No Match: Add a new Symbol to the list.
    TL::ObjectList<TL::Type> param_type_list;
    param_type_list.append(types[0]);
    
    result = (scope_entry_t*) calloc(1, sizeof(scope_entry_t));
    result->symbol_name = BUILTIN_VR_NAME;
    result->kind = SK_FUNCTION;
    result->type_information = ((TL::Type)types[0])
        .get_generic_vector_to()
        .get_function_returning(param_type_list)
        .get_internal_type();
    result->decl_context = builtin_vr_list.at(0).get_internal_symbol()->decl_context;
    //BUILTIN + MUTABLE = LTYPE!
    result->entity_specs.is_builtin = 1;
    result->entity_specs.is_mutable = 1;

    builtin_vr_list.append(TL::Symbol(result));

    return result;
}


//__builtin_generic_function overload
static scope_entry_t* solve_generic_func_overload_name(scope_entry_t* overloaded_function, 
        type_t** types,  
        AST *arguments UNUSED_PARAMETER,
        int num_arguments)
{
    char name[256];
    int i;
    char found_match = 0;
    scope_entry_t* result = NULL;

    for(i=1; i<builtin_gf_list.size(); i++) 
    {
        if (equivalent_types(get_unqualified_type(types[0]),
                    function_type_get_parameter_type_num(builtin_gf_list[i].get_type()
                        .get_internal_type(), 0)))
        {
            return builtin_gf_list[i].get_internal_symbol();
        }
    }

    //No Match: Add a new Symbol to the list.
    TL::ObjectList<TL::Type> param_type_list;
    for(i=0; i<num_arguments; i++)
    {
        param_type_list.append(types[i]);
    }

    result = (scope_entry_t*) calloc(1, sizeof(scope_entry_t));
    result->symbol_name = BUILTIN_GF_NAME;
    result->kind = SK_FUNCTION;
    result->type_information = TL::Type(types[0])
        .returns()
        .get_generic_vector_to()
        .get_function_returning(param_type_list)
        .get_internal_type();
    result->decl_context = builtin_gf_list.at(0).get_internal_symbol()->decl_context;
    result->entity_specs.is_builtin = 1;

    builtin_gf_list.append(result);

    return result;
}


//__builtin_vector_conversion overload
static scope_entry_t* solve_vector_conv_overload_name(scope_entry_t* overloaded_function, 
        type_t** types,  
        AST *arguments UNUSED_PARAMETER,
        int num_arguments)
{
    char name[256];
    int i;
    char found_match = 0;
    scope_entry_t* result = NULL;

    if (num_arguments != 3)
    {
        internal_error("hlt-simd builtin '%s' only allows two parameter\n",
                                overloaded_function->symbol_name);
    }

    for(i=1; i<builtin_vc_list.size(); i++) 
    {
        if (equivalent_types(get_unqualified_type(types[1]),
                    function_type_get_parameter_type_num(builtin_vc_list[i].get_type()
                        .get_internal_type(), 1)))
        {
            return builtin_vc_list[i].get_internal_symbol();
        }
    }

    //No Match: Add a new Symbol to the list.
    TL::ObjectList<TL::Type> param_type_list;

    param_type_list.append(types[0]);
    param_type_list.append(types[1]);
    param_type_list.append(types[2]);

    result = (scope_entry_t*) calloc(1, sizeof(scope_entry_t));
    result->symbol_name = BUILTIN_VC_NAME;
    result->kind = SK_FUNCTION;

    //Last symbol: target type of the conversion
    result->type_information = TL::Type(types[1])
        .get_function_returning(param_type_list)
        .get_internal_type();
    result->decl_context = builtin_vc_list.at(0).get_internal_symbol()->decl_context;
    result->entity_specs.is_builtin = 1;

    builtin_vc_list.append(result);

    return result;
}


static scope_entry_t* solve_vector_exp_overload_name(scope_entry_t* overloaded_function,
        type_t** types,  
        AST *arguments UNUSED_PARAMETER,
        int num_arguments)
{
    char name[256];
    int i;
    char found_match = 0;
    scope_entry_t* result = NULL;

    if (num_arguments != 1)
    {
        internal_error("hlt-simd builtin '%s' only allows one parameter\n", 
                        overloaded_function->symbol_name);
    }

    for(i=1; i<builtin_ve_list.size(); i++) 
    {
        type_t* first_param_type = function_type_get_parameter_type_num(builtin_ve_list[i].get_type().get_internal_type(), 0);

        if (equivalent_types(get_unqualified_type(types[0]), first_param_type))
        {
            return builtin_ve_list[i].get_internal_symbol();
        }
    }

    //No Match: Add a new Symbol to the list.
    TL::ObjectList<TL::Type> params_list;
    params_list.append(types[0]);
    
    result = (scope_entry_t*) calloc(1, sizeof(scope_entry_t));
    result->symbol_name = BUILTIN_VE_NAME;
    result->kind = SK_FUNCTION;
    result->type_information = ((TL::Type)types[0]).get_generic_vector_to()
                                                   .get_function_returning(params_list)
                                                   .get_internal_type();
    result->decl_context = builtin_ve_list.at(0).get_internal_symbol()->decl_context;

    builtin_ve_list.append(result);

    return result;
}


HLTPragmaPhase::HLTPragmaPhase()
    : PragmaCustomCompilerPhase("hlt")
{
    set_phase_name("High Level Transformations");
    set_phase_description("This phase implements several high level "
            "transformations available through the usage of #pragma hlt");

    register_construct("unroll");
    on_directive_post["unroll"].connect(functor(&HLTPragmaPhase::unroll_loop, *this));

    register_construct("block");
    on_directive_post["block"].connect(functor(&HLTPragmaPhase::block_loop, *this));

    register_construct("blocking");
    on_directive_post["blocking"].connect(functor(&HLTPragmaPhase::block_loop, *this));

    register_construct("stripmine");
    on_directive_post["stripmine"].connect(functor(&HLTPragmaPhase::stripmine_loop, *this));

    register_construct("distribute");
    on_directive_post["distribute"].connect(functor(&HLTPragmaPhase::distribute_loop, *this));

    register_construct("fusion");
    on_directive_pre["fusion"].connect(functor(&HLTPragmaPhase::pre_fuse_loops, *this));
    on_directive_post["fusion"].connect(functor(&HLTPragmaPhase::fuse_loops, *this));

    register_construct("interchange");
    on_directive_post["interchange"].connect(functor(&HLTPragmaPhase::interchange_loops, *this));

    register_construct("collapse");
    on_directive_post["collapse"].connect(functor(&HLTPragmaPhase::collapse_loop, *this));

    register_construct("outline");
    on_directive_post["outline"].connect(functor(&HLTPragmaPhase::outline_code, *this));

    register_construct("extend");
    on_directive_post["extend"].connect(functor(&HLTPragmaPhase::extend_function, *this));

    register_construct("peel");
    on_directive_post["peel"].connect(functor(&HLTPragmaPhase::peel_loop, *this));

    register_construct("task_aggregate");
    on_directive_post["task_aggregate"].connect(functor(&HLTPragmaPhase::task_aggregate, *this));

    register_construct("simd");
    on_directive_post["simd"].connect(functor(&HLTPragmaPhase::simdize, *this));

    _allow_identity_str = "1";

    register_parameter("allow_identity", 
            "Use this to disable identity, this is for testing only",
            _allow_identity_str,
            "true").connect(functor( update_identity_flag ));

    register_parameter("instrument",
            "Enables mintaka instrumentation if set to '1'",
            _enable_hlt_instr_str,
            "0").connect(functor( &HLTPragmaPhase::set_instrument_hlt, *this ));
}

void HLTPragmaPhase::set_instrument_hlt(const std::string &str)
{
    TL::parse_boolean_option("instrument",
            str,
            HLT::enable_instrumentation,
            "Option 'instrument' is a boolean flag");
}

//FIXME: Move me to a new phase
void HLTPragmaPhase::simd_pre_run(AST_t translation_unit,
        ScopeLink scope_link)
{
    Scope global_scope = scope_link.get_scope(translation_unit);

    //New Artificial Symbol: __builtin_vector_reference
    Symbol builtin_vr_sym = global_scope.new_artificial_symbol(BUILTIN_VR_NAME);
    scope_entry_t* builtin_vr_se = builtin_vr_sym.get_internal_symbol();
    builtin_vr_se->kind = SK_FUNCTION;
    //BUILTIN + MUTABLE = LTYPE
    builtin_vr_se->entity_specs.is_builtin = 1;
    builtin_vr_se->entity_specs.is_mutable = 1;
    builtin_vr_se->type_information = get_computed_function_type(solve_vector_ref_overload_name);
    //Artificial Symbol in list[0]
    builtin_vr_list.append(builtin_vr_sym);

    //new artificial symbol: __builtin_vector_expansion
    Symbol builtin_ve_sym = global_scope.new_artificial_symbol(BUILTIN_VE_NAME);
    scope_entry_t* builtin_ve_se = builtin_ve_sym.get_internal_symbol();
    builtin_ve_se->kind = SK_FUNCTION;
    builtin_ve_se->entity_specs.is_builtin = 1;
    builtin_ve_se->type_information = get_computed_function_type(solve_vector_exp_overload_name);
    //artificial symbol in list[0]
    builtin_ve_list.append(builtin_ve_sym);

    //new artificial symbol: __builtin_global_function
    Symbol builtin_gf_sym = global_scope.new_artificial_symbol(BUILTIN_GF_NAME);
    scope_entry_t* builtin_gf_se = builtin_gf_sym.get_internal_symbol();
    builtin_gf_se->kind = SK_FUNCTION;
    builtin_gf_se->entity_specs.is_builtin = 1;
    builtin_gf_se->type_information = get_computed_function_type(solve_generic_func_overload_name);
    //artificial symbol in list[0]
    builtin_gf_list.append(builtin_gf_sym);

    //new artificial symbol: __builtin_vector_conversion
    Symbol builtin_vc_sym = global_scope.new_artificial_symbol(BUILTIN_VC_NAME);
    scope_entry_t* builtin_vc_se = builtin_vc_sym.get_internal_symbol();
    builtin_vc_se->kind = SK_FUNCTION;
    builtin_vc_se->entity_specs.is_builtin = 1;
    builtin_vc_se->type_information = get_computed_function_type(solve_vector_conv_overload_name);
    //artificial symbol in list[0]
    builtin_vc_list.append(builtin_vc_sym);


    Source intel_builtins_src, scalar_functions_src, default_generic_functions_src, conversions_src;

    scalar_functions_src
        << "extern int abs (int __x) __attribute__ ((__nothrow__));"

        << "extern float sqrtf (float __x) __attribute__ ((__nothrow__));"
        << "extern float fabsf (float __x) __attribute__ ((__nothrow__));"

        << "extern double sqrt (double __x) __attribute__ ((__nothrow__));"
        << "extern double fabs (double __x) __attribute__ ((__nothrow__));"
        ;

    default_generic_functions_src
        << "static inline float __attribute__((generic_vector)) __fabsf_default (float __attribute__((generic_vector)) a)"
        << "{"
        <<      "return (float __attribute__((generic_vector))) (((int __attribute__((generic_vector)))a) &"
        <<      "__builtin_vector_expansion(0x7FFFFFFF));"
        << "}"

        << "static inline double __attribute__((generic_vector)) __fabs_default (double __attribute__((generic_vector)) a)"
        << "{"
        <<      "return (double __attribute__((generic_vector))) (((long long int __attribute__((generic_vector)))a) &"
        <<      "__builtin_vector_expansion(0x7FFFFFFFFFFFFFFFLL));"
        << "}"
        ;

    intel_builtins_src
    //SSE2
        << "char __attribute__((vector_size(16)))           __builtin_ia32_pabsb128 (char __attribute__((vector_size(16))));"
        << "unsigned char __attribute__((vector_size(16)))  __builtin_ia32_packuswb128(unsigned short int __attribute__((vector_size(16))) vs0, unsigned short int __attribute__((vector_size(16))) vs1);"
        << "char __attribute__((vector_size(16)))           __builtin_ia32_packsswb128(short int __attribute__((vector_size(16))) vs0, short int __attribute__((vector_size(16))) vs1);"
        //<< "short int __attribute__((vector_size(16)))    __builtin_ia32_pabsw128 (short int __attribute__((vector_size(16))));"
        << "int __attribute__((vector_size(16)))            __builtin_ia32_pabsd128 (int __attribute__((vector_size(16))));"
        << "int __attribute__((vector_size(16)))            __builtin_ia32_cmpltps (float __attribute__((vector_size(16))), float __attribute__((vector_size(16))));"
        << "float __attribute__((vector_size(16)))          __builtin_ia32_sqrtps (float __attribute__((vector_size(16))));"
        //<< "float __attribute__((vector_size(16)))        __builtin_ia32_rsqrtps (float __attribute__((vector_size(16))));"
        << "double __attribute__((vector_size(16)))         __builtin_ia32_sqrtpd (double __attribute__((vector_size(16))));"
        //<< "double __attribute__((vector_size(16)))       __builtin_ia32_rsqrtpd (double __attribute__((vector_size(16))));"
    //SSSE3
        << "int __attribute__((vector_size(16)))            __builtin_ia32_pabsd128 (int __attribute__((vector_size(16))));"
        << "int __attribute__((vector_size(16)))            __builtin_ia32_cvttps2dq(float __attribute__((vector_size(16))));"
    //SSE4.1
        << "unsigned short int __attribute__((vector_size(16))) __builtin_ia32_packusdw128(int __attribute__((vector_size(16))), int __attribute__((vector_size(16))));"
        ;

    conversions_src
        << "static inline char __attribute__((vector_size(16))) " << COMPILER_CONV_FLOAT2CHAR_SMP16 << "("
        <<      "float __attribute__((vector_size(16))) vf0,"
        <<      "float __attribute__((vector_size(16))) vf1," 
        <<      "float __attribute__((vector_size(16))) vf2," 
        <<      "float __attribute__((vector_size(16))) vf3)"
        << "{"
        <<      "int __attribute__((vector_size(16))) vi0, vi1;"
        <<      "short int __attribute__((vector_size(16))) vs0, vs1;"
        <<      "vi0 = __builtin_ia32_cvttps2dq(vf0);"
        <<      "vi1 = __builtin_ia32_cvttps2dq(vf1);"
        <<      "vs0 = __builtin_ia32_packssdw128(vi0, vi1);"
        <<      "vi0 = __builtin_ia32_cvttps2dq(vf2);"
        <<      "vi1 = __builtin_ia32_cvttps2dq(vf3);"
        <<      "vs1 = __builtin_ia32_packssdw128(vi0, vi1);"
        <<      "return __builtin_ia32_packsswb128(vs0, vs1);"
        << "}"

        << "static inline unsigned char __attribute__((vector_size(16))) " << COMPILER_CONV_FLOAT2UCHAR_SMP16 << "("
        <<      "float __attribute__((vector_size(16))) vf0,"
        <<      "float __attribute__((vector_size(16))) vf1," 
        <<      "float __attribute__((vector_size(16))) vf2," 
        <<      "float __attribute__((vector_size(16))) vf3)"
        << "{"
        <<      "int __attribute__((vector_size(16))) vi0, vi1;"
        <<      "unsigned short int __attribute__((vector_size(16))) vs0, vs1;"
        <<      "vi0 = __builtin_ia32_cvttps2dq(vf0);"
        <<      "vi1 = __builtin_ia32_cvttps2dq(vf1);"
        <<      "vs0 = __builtin_ia32_packusdw128(vi0, vi1);"
        <<      "vi0 = __builtin_ia32_cvttps2dq(vf2);"
        <<      "vi1 = __builtin_ia32_cvttps2dq(vf3);"
        <<      "vs1 = __builtin_ia32_packusdw128(vi0, vi1);"
        <<      "return __builtin_ia32_packuswb128(vs0, vs1);"
        << "}"

        ;

    //Global parsing
    scalar_functions_src.parse_global(translation_unit, scope_link);
    default_generic_functions_src.parse_global(translation_unit, scope_link);
    intel_builtins_src.parse_global(translation_unit, scope_link);
    conversions_src.parse_global(translation_unit, scope_link);

    Scope scope = scope_link.get_scope(translation_unit);

    //Default functions
    std::string device_name = "smp";
    int width = 16;

    //Char
//    generic_functions.add_specific_definition(scope.get_symbol_from_name("_float_to_char_smp16"), TL::SIMD::DEFAULT, device_name, width, false, true, std::string("_float_to_char_smp16"));

    //Int
    generic_functions.add_specific_definition(scope.get_symbol_from_name("abs"), TL::SIMD::ARCH_DEFAULT, device_name, width, false, false, std::string("__builtin_ia32_pabsd128"));

    //Float
    generic_functions.add_specific_definition(scope.get_symbol_from_name("sqrtf"), TL::SIMD::ARCH_DEFAULT, device_name, width, false, false, std::string("__builtin_ia32_sqrtps"));
    //generic_functions.add_specific_definition(scope.get_symbol_from_name("rsqrtf"), TL::SIMD::DEFAULT, device_name, width, false, std::string("__builtin_ia32_rsqrtps"));
    generic_functions.add_generic_function(scope.get_symbol_from_name("fabsf"), scope.get_symbol_from_name("__fabsf_default"));
    generic_functions.add_specific_definition(scope.get_symbol_from_name("fabsf"), TL::SIMD::SIMD, device_name, width, false, true);

    generic_functions.add_specific_definition(scope.get_symbol_from_name("fabsf"), TL::SIMD::SIMD, device_name, width, false, true);


    //Double
    generic_functions.add_specific_definition(scope.get_symbol_from_name("sqrt"), TL::SIMD::ARCH_DEFAULT, device_name, width, false, false, std::string("__builtin_ia32_sqrtpd"));
    //generic_functions.add_specific_definition(scope.get_symbol_from_name("rsqrt"), TL::SIMD::DEFAULT, device_name, width, false, std::string("__builtin_ia32_rsqrtpd"));
    generic_functions.add_generic_function(scope.get_symbol_from_name("fabs"), scope.get_symbol_from_name("__fabs_default"));
    generic_functions.add_specific_definition(scope.get_symbol_from_name("fabs"), TL::SIMD::SIMD, device_name, width, false, true);
}

void HLTPragmaPhase::run(TL::DTO& dto)
{
    try
    {
        PragmaCustomCompilerPhase::run(dto);
    }
    catch (HLTException e)
    {
        std::cerr << e << std::endl;
        set_phase_status(PHASE_STATUS_ERROR);
    }
    catch (...)
    {
        std::cerr << "(hlt-phase): error: unknown exception" << std::endl;
        set_phase_status(PHASE_STATUS_ERROR);
    }
}

struct UnrollInfo
{
    int factor;
    bool enable_omp_bundling;
    bool ignore_omp;
	int omp_bundling_factor;
	bool remove_tasks;
	bool timing;
    bool aggregate_epilog;

    UnrollInfo()
        : factor(4), 
        enable_omp_bundling(false),
        ignore_omp(false),
		omp_bundling_factor(-1),
		remove_tasks(false),
		timing(false),
        aggregate_epilog(false)
    {
    }
};

static void unroll_loop_fun(TL::ForStatement for_stmt,
        UnrollInfo unroll_info)
{
    TL::Source unrolled_loop_src = TL::HLT::unroll_loop(for_stmt,  unroll_info.factor)
        .ignore_omp(unroll_info.ignore_omp)
        .enable_omp_bundling(unroll_info.enable_omp_bundling)
		.set_omp_bundling_factor(unroll_info.omp_bundling_factor)
		.set_remove_tasks(unroll_info.remove_tasks)
		.set_timing(unroll_info.timing)
        .set_omp_aggregate_epilog(unroll_info.aggregate_epilog)
        .allow_identity(_allow_identity);

    TL::AST_t unrolled_loop_tree = unrolled_loop_src.parse_statement(for_stmt.get_ast(),
            for_stmt.get_scope_link());

    for_stmt.get_ast().replace(unrolled_loop_tree);
}

void HLTPragmaPhase::unroll_loop(PragmaCustomConstruct construct)
{
    Statement statement = construct.get_statement();

    TL::PragmaCustomClause factor_clause = construct.get_clause("factor");

    int unroll_factor = 32;
    if (factor_clause.is_defined())
    {
        ObjectList<Expression> args = factor_clause.get_expression_list();

        if (args.size() != 1)
        {
            throw HLTException(construct, "factor clause only accepts one argument");
        }

        Expression& expr = args[0];

        if (!expr.is_constant())
        {
            throw HLTException(expr, "factor clause argument should be a constant expression");
        }

        bool valid = false;
        unroll_factor = expr.evaluate_constant_int_expression(valid);
        if (!valid)
        {
            throw HLTException(expr, "factor clause argument expression could not be evaluated");
        }
    }
    else
    {
        std::cerr << construct.get_ast().get_locus() << ": warning: no factor clause given for unrolling, assuming 'factor(32)'" << std::endl;
    }

    ObjectList<ForStatement> for_statement_list = get_all_sibling_for_statements(statement);

    if (!_allow_identity
            && for_statement_list.empty())
    {
        throw HLTException(construct, "not found any suitable construct for this pragma");
    }

    UnrollInfo unroll_info;
    unroll_info.factor = unroll_factor;


    if (construct.get_clause("ignore_omp").is_defined())
    {
        unroll_info.ignore_omp = true;
    }
    if (construct.get_clause("omp_bundling").is_defined())
    {
        unroll_info.enable_omp_bundling = true;
    }
	if (construct.get_clause("remove_tasks").is_defined())
	{
		unroll_info.remove_tasks = true;
	}
	if (construct.get_clause("aggregate_epilog").is_defined())
	{
		unroll_info.aggregate_epilog = true;
	}
	if (construct.get_clause("timing").is_defined())
	{
		unroll_info.timing = true;
		std::cerr << "TIMING clause found" << std::endl;
	}
	if (construct.get_clause("omp_bundling_factor").is_defined())
	{
		TL::PragmaCustomClause bundling_factor = construct.get_clause("omp_bundling_factor");
        ObjectList<Expression> args = bundling_factor.get_expression_list();

        if (args.size() != 1)
        {
            throw HLTException(construct, "omp_bundling_factor clause only accepts one argument");
        }

        Expression& expr = args[0];

        if (!expr.is_constant())
        {
            throw HLTException(expr, "omp_bundling_factor clause argument should be a constant expression");
        }

        bool valid = false;
        unroll_info.omp_bundling_factor = expr.evaluate_constant_int_expression(valid);
        if (!valid)
        {
            throw HLTException(expr, "omp_bundling_factor clause argument expression could not be evaluated");
        }
	}

    std::for_each(for_statement_list.begin(), for_statement_list.end(),
            std::bind2nd(std::ptr_fun(unroll_loop_fun), unroll_info));

    construct.get_ast().replace(statement.get_ast());
}

static void block_loop_fun(TL::ForStatement for_stmt, 
        TL::ObjectList<TL::Expression> factors_list)
{
    // ForStatement &for_stmt(*it);
    TL::Source blocked_loop_src = TL::HLT::block_loop(for_stmt, factors_list).allow_identity(_allow_identity);

    TL::AST_t blocked_loop_tree = blocked_loop_src.parse_statement(for_stmt.get_ast(),
            for_stmt.get_scope_link());

    for_stmt.get_ast().replace(blocked_loop_tree);
}

void HLTPragmaPhase::block_loop(PragmaCustomConstruct construct)
{
    Statement statement = construct.get_statement();

    ObjectList<ForStatement> for_statement_list = get_all_sibling_for_statements(statement);

    TL::PragmaCustomClause factors_clause = construct.get_clause("factors");

    if (!factors_clause.is_defined())
    {
        throw HLTException(construct, "'#pragma hlt block' requires a clause 'factors' with a list of block factors");
    }

    if (!_allow_identity
            && for_statement_list.empty())
    {
        throw HLTException(construct, "not found any suitable construct for this pragma");
    }

    TL::ObjectList<TL::Expression> factors_list = factors_clause.get_expression_list();
    std::for_each(for_statement_list.begin(), for_statement_list.end(),
            std::bind2nd(std::ptr_fun(block_loop_fun), factors_list));

    // Remove the pragma
    construct.get_ast().replace(statement.get_ast());
}

static void stripmine_loop_fun(TL::ForStatement for_stmt,
        TL::PragmaCustomClause amount_clause)
{
    TL::Source amount;
    amount << amount_clause.get_arguments()[0];

    TL::Source stripmined_loop_src = TL::HLT::stripmine_loop(for_stmt, amount).allow_identity(_allow_identity);

    TL::AST_t stripmined_loop_tree = stripmined_loop_src.parse_statement(for_stmt.get_ast(),
            for_stmt.get_scope_link());

    for_stmt.get_ast().replace(stripmined_loop_tree);
}


void HLTPragmaPhase::stripmine_loop(PragmaCustomConstruct construct)
{
    Statement statement = construct.get_statement();

    ObjectList<ForStatement> for_statement_list = get_all_sibling_for_statements(statement);

    TL::PragmaCustomClause amount_clause = construct.get_clause("amount");

    if (!amount_clause.is_defined())
    {
        throw HLTException(construct, "'#pragma hlt stripmine' requires a clause 'amount' with the amount of stripmining");
    }

    if (!_allow_identity
                && for_statement_list.empty())
    {
        throw HLTException(construct, "not found any suitable construct for this pragma");
    }

    std::for_each(for_statement_list.begin(), for_statement_list.end(),
            std::bind2nd(std::ptr_fun(stripmine_loop_fun), amount_clause));

    // Remove the pragma
    construct.get_ast().replace(statement.get_ast());
}

void distribute_loop_fun(TL::ForStatement for_stmt,
        TL::ObjectList<TL::Symbol> expanded_syms)
{
    TL::Source distributed_loop_src = TL::HLT::distribute_loop(for_stmt, expanded_syms).allow_identity(_allow_identity);

    TL::AST_t distributed_loop_tree = distributed_loop_src.parse_statement(for_stmt.get_ast(),
            for_stmt.get_scope_link());

    for_stmt.get_ast().replace(distributed_loop_tree);
}

void HLTPragmaPhase::distribute_loop(PragmaCustomConstruct construct)
{
    Statement statement = construct.get_statement();

    ObjectList<ForStatement> for_statement_list = get_all_sibling_for_statements(statement);

    PragmaCustomClause expanded_scalars = construct.get_clause("expand");

    ObjectList<Symbol> expanded_syms;
    if (expanded_scalars.is_defined())
    {
        ObjectList<IdExpression> id_expression_list = expanded_scalars.id_expressions();

        // FIXME - Figure a nice way to yield this error
        // if (!id_expression_list.empty())
        // {
        //     if (!for_stmt.is_regular_loop())
        //     {
        //         throw HLTException(construct, 
        //                 "'#pragma hlt distribute' when scalar expansion is requested can "
        //                 "only be used with regular for-statements");
        //     }
        // }
        expanded_syms.insert(id_expression_list.map(functor(&IdExpression::get_symbol)));
    }

    if (!_allow_identity
            && for_statement_list.empty())
    {
        throw HLTException(construct, "not found any suitable construct for this pragma");
    }

    std::for_each(for_statement_list.begin(), for_statement_list.end(),
            std::bind2nd(std::ptr_fun(distribute_loop_fun), expanded_syms));

    construct.get_ast().replace(statement.get_ast());
}

void HLTPragmaPhase::pre_fuse_loops(PragmaCustomConstruct construct)
{
    Statement statement = construct.get_statement();

    bool &is_jam = construct.get_data<bool>("perform_jam");
    is_jam = false;

    if (is_pragma_custom_construct("hlt", "unroll", statement.get_ast(), construct.get_scope_link()))
    {
        PragmaCustomConstruct pragma_construct(statement.get_ast(), construct.get_scope_link());
        Statement inner_stmt = pragma_construct.get_statement();

        if (ForStatement::predicate(inner_stmt.get_ast()))
        {
            ForStatement inner_for_statement(inner_stmt.get_ast(), construct.get_scope_link());

            Statement loop_body = inner_for_statement.get_loop_body();

            if (ForStatement::predicate(loop_body.get_ast()))
            {
                is_jam = true;
            }
        }
    }
}

void HLTPragmaPhase::fuse_loops(PragmaCustomConstruct construct)
{
    Statement statement = construct.get_statement();

    AST_t result_tree = statement.get_ast();

    bool &is_jam = construct.get_data<bool>("perform_jam");

    if (is_jam)
    {
        // We have to jam the loops
        jam_loops(statement);
    }
    else if (statement.is_compound_statement())
    {
        ObjectList<Statement> statement_list = statement.get_inner_statements();

        ObjectList<ForStatement> for_statement_list;

        Source kept_statements;

        for (ObjectList<Statement>::iterator it = statement_list.begin();
                it != statement_list.end();
                it++)
        {
            if (ForStatement::predicate(it->get_ast()))
            {
                for_statement_list.append(ForStatement(it->get_ast(), it->get_scope_link()));
            }
            else
            {
                kept_statements << (*it);
            }
        }

        if (!_allow_identity
                && for_statement_list.empty())
        {
            throw HLTException(construct, "not found any suitable construct for this pragma");
        }

        if (for_statement_list.size() > 1)
        {

            TL::Source fused_loops_src = HLT::loop_fusion(for_statement_list);

            Source result;

            result 
                << "{"
                << fused_loops_src
                << kept_statements
                << "}"
                ;

            result_tree = result.parse_statement(
                    for_statement_list[0].get_ast(),
                    construct.get_scope_link());
        }
    }

    construct.get_ast().replace(result_tree);
}

static int evaluate_expr(TL::Expression expr)
{
    if (expr.is_constant())
    {
        bool valid;
        return expr.evaluate_constant_int_expression(valid);
    }
    else
    {
        return -1;
    }
}

void interchange_loops_fun(TL::ForStatement for_stmt,
        TL::ObjectList<int> permutation_list)
{
    TL::Source interchange_src = TL::HLT::loop_interchange(for_stmt, permutation_list).allow_identity(_allow_identity);

    TL::AST_t interchange_tree = interchange_src.parse_statement(for_stmt.get_ast(),
            for_stmt.get_scope_link());

    for_stmt.get_ast().replace(interchange_tree);
}

void HLTPragmaPhase::interchange_loops(PragmaCustomConstruct construct)
{
    TL::Statement statement = construct.get_statement();

    TL::ObjectList<TL::ForStatement> for_statement_list = get_all_sibling_for_statements(statement);

    PragmaCustomClause permutation = construct.get_clause("permutation");

    if (!permutation.is_defined())
    {
        throw HLTException(construct, "'#pragma hlt interchange' requires a 'permutation' clause");
    }

    ObjectList<Expression> expr_list = permutation.get_expression_list();

    // Now evaluate every expression
    TL::ObjectList<int> permutation_list;
    std::transform(expr_list.begin(), expr_list.end(), std::back_inserter(permutation_list), evaluate_expr);

    if (!_allow_identity
            && for_statement_list.empty())
    {
        throw HLTException(construct, "not found any suitable construct for this pragma");
    }

    std::for_each(for_statement_list.begin(), for_statement_list.end(),
            std::bind2nd(std::ptr_fun(interchange_loops_fun), permutation_list));

    construct.get_ast().replace(statement.get_ast());
}

void collapse_loop_fun(TL::ForStatement for_stmt, int level)
{
    TL::HLT::LoopCollapse loop_collapse = TL::HLT::loop_collapse(for_stmt);
    loop_collapse.allow_identity(_allow_identity);

    if (level != 0)
    {
        loop_collapse.set_nesting_level(level);
    }

    TL::Source collapsed_loop_src = loop_collapse;
    TL::AST_t collapsed_loop_tree = collapsed_loop_src.parse_statement(for_stmt.get_ast(),
            for_stmt.get_scope_link());

    for_stmt.get_ast().replace(collapsed_loop_tree);

}

void HLTPragmaPhase::collapse_loop(PragmaCustomConstruct construct)
{
    Statement statement = construct.get_statement();

    ObjectList<ForStatement> for_statement_list = get_all_sibling_for_statements(statement);

    if (!_allow_identity
            && for_statement_list.empty())
    {
        throw HLTException(construct, "not found any suitable construct for this pragma");
    }

    int nest_level = 0;
    PragmaCustomClause level = construct.get_clause("level");
    if (level.is_defined())
    {
        ObjectList<Expression> expr_list = level.get_expression_list();
        if (expr_list.size() != 1)
        {
            throw HLTException(construct, "clause 'level' requires only one argument");
        }
        bool valid = false;
        nest_level = expr_list[0].evaluate_constant_int_expression(valid);

        if (!valid)
        {
            throw HLTException(construct, "invalid value '" + expr_list[0].prettyprint() + "' for 'level' clause");
        }
    }

    std::for_each(for_statement_list.begin(), for_statement_list.end(),
            std::bind2nd(std::ptr_fun(collapse_loop_fun), nest_level));

    construct.get_ast().replace(statement.get_ast());
}

static std::string prettyprint_without_braces(TL::Statement st)
{
    std::string result;
    if (st.is_compound_statement())
    {
        TL::ObjectList<TL::Statement> inner_st = st.get_inner_statements();
        for (TL::ObjectList<TL::Statement>::iterator it = inner_st.begin();
                it != inner_st.end();
                it++)
        {
            result += it->prettyprint();
        }
    }
    else
        result = st.prettyprint();

    return result;
}

void HLTPragmaPhase::jam_loops(Statement unrolled_loop_code)
{
    if (!unrolled_loop_code.is_compound_statement())
    {
        throw HLTException(unrolled_loop_code, "This should be a compound statement");
    }

    // This is a bit fragile: most of the time main_loop will be either 0 or 1
    int main_loop = 0;
    {
        ObjectList<Statement> inner_statements = unrolled_loop_code.get_inner_statements();
        for (ObjectList<Statement>::iterator it = inner_statements.begin();
                it != inner_statements.end();
                it++)
        {
            if (ForStatement::predicate(it->get_ast()))
            {
                break;
            }
            main_loop++;
        }
    }

    Statement unrolled_loop(unrolled_loop_code.get_inner_statements()[main_loop].get_ast(),
            unrolled_loop_code.get_scope_link());

    if (!ForStatement::predicate(unrolled_loop.get_ast()))
    {
        throw HLTException(unrolled_loop, "This should be a for-statement");
    }
    ForStatement for_stmt(unrolled_loop.get_ast(), unrolled_loop.get_scope_link());
    Statement loop_body = for_stmt.get_loop_body();

    if (!loop_body.is_compound_statement())
    {
        throw HLTException(loop_body, "This should be a compound-statement");
    }

    ObjectList<Statement> inner_statements = loop_body.get_inner_statements();

    Source jammed_loop_bodies;

    for (ObjectList<Statement>::iterator it = inner_statements.begin();
            it != inner_statements.end();
            it++)
    {
        if (!ForStatement::predicate(it->get_ast()))
        {
            throw HLTException(*it, "This should be a for-statement");
        }

        ForStatement current_for(it->get_ast(), unrolled_loop.get_scope_link());

        jammed_loop_bodies
            << prettyprint_without_braces(current_for.get_loop_body())
            ;
    }

    Source for_header;

    // Assumption: for does not depend on the enclosing induction variable
    {
        ForStatement pattern_for(inner_statements[0].get_ast(), unrolled_loop.get_scope_link());

        for_header
            << "for(" << pattern_for.get_iterating_init().prettyprint() 
            << pattern_for.get_iterating_condition() << ";"
            << pattern_for.get_iterating_expression() << ")"
            ;
    }

    Source result;

    result
        << for_header
        << "{"
        << jammed_loop_bodies
        << "}"
        ;

    AST_t jammed_tree = result.parse_statement(loop_body.get_ast(),
            loop_body.get_scope_link());

    loop_body.get_ast().replace(jammed_tree);
}

void HLTPragmaPhase::outline_code(PragmaCustomConstruct construct)
{
    Statement stmt = construct.get_statement();

    TL::HLT::Outline outline(construct.get_scope_link(), stmt);

    PragmaCustomClause packed = construct.get_clause("packed");
    if (packed.is_defined())
    {
        outline.use_packed_arguments();
    }

    PragmaCustomClause name = construct.get_clause("name");
    if (name.is_defined())
    {
        ObjectList<std::string> clause_args = name.get_arguments(ExpressionTokenizer());
        outline.set_outline_name(clause_args[0]);
    }

    Source src = outline;
}

void HLTPragmaPhase::extend_function(PragmaCustomConstruct construct)
{
    AST_t decl = construct.get_declaration();
    if (!FunctionDefinition::predicate(decl))
    {
        throw HLTException(construct, "'#pragma hlt extend' must be followed by a function-definition");
    }


    FunctionDefinition funct_def(decl, construct.get_scope_link());

    TL::HLT::FunctionExtension funct_extensions(funct_def);

    PragmaCustomClause factor_clause = construct.get_clause("factor");
    if (factor_clause.is_defined())
    {
        Expression factor = factor_clause.get_expression_list()[0];
        funct_extensions.set_extension_amount(factor);
    }


    PragmaCustomClause name_clause = construct.get_clause("name");
    if (name_clause.is_defined())
    {
        ObjectList<std::string> clause_args = name_clause.get_arguments(ExpressionTokenizer());
        funct_extensions.set_extended_function_name(clause_args[0]);
    }

    Source src = funct_extensions;

    TL::AST_t new_function = src.parse_declaration(construct.get_ast(),
            construct.get_scope_link());

    construct.get_ast().prepend(new_function);

    // Now remove the pragma
    construct.get_ast().replace(decl);
}

void HLTPragmaPhase::peel_loop(PragmaCustomConstruct construct)
{
    Statement stmt = construct.get_statement();

    if (!ForStatement::predicate(stmt.get_ast()))
    {
        throw HLTException(construct, "'#pragma hlt peel' must be followed by a for-statement");
    }

    ForStatement for_statement(stmt.get_ast(), stmt.get_scope_link());

    PragmaCustomClause init_peel_clause = construct.get_clause("start");
    PragmaCustomClause end_peel_clause = construct.get_clause("end");

    if (!init_peel_clause.is_defined() && !end_peel_clause.is_defined())
    {
        throw HLTException(construct, "'#pragma hlt peel' requires at least a clause 'start(N)' or 'end(N)'");
    }

    int init_peel = 0;
    int end_peel = 0;

    if (init_peel_clause.is_defined())
    {
        bool valid = true;

        ObjectList<Expression> args = init_peel_clause.get_expression_list();

        if (args.size() != 1)
        {
            throw HLTException(init_peel_clause, "'start' clause requires one argument");
        }

        Expression &factor = args[0];

        init_peel = factor.evaluate_constant_int_expression(valid);

        if (!valid)
        {
            throw HLTException(factor, "invalid constant expression in clause 'start'");
        }
    }

    if (end_peel_clause.is_defined())
    {
        bool valid = true;

        ObjectList<Expression> args = end_peel_clause.get_expression_list();

        if (args.size() != 1)
        {
            throw HLTException(end_peel_clause, "'end' clause requires one argument");
        }

        Expression &factor = args[0];

        end_peel = factor.evaluate_constant_int_expression(valid);

        if (!valid)
        {
            throw HLTException(factor, "invalid constant expression in clause 'end'");
        }
    }

    Source src = loop_peeling(for_statement, init_peel, end_peel);

    AST_t tree = src.parse_statement(construct.get_ast(),
            construct.get_scope_link());

    construct.get_ast().replace(tree);
}

void HLTPragmaPhase::task_aggregate(PragmaCustomConstruct construct)
{
    Statement stmt = construct.get_statement();

    TaskAggregation task_aggregation(stmt);


    Source src, aggregation, global, finish;
    src
        << global
        << aggregation
        << finish
        ;

    if (construct.get_clause("omp_bundling").is_defined())
    {
        task_aggregation.set_aggregation_method(TaskAggregation::BUNDLING)
            .set_global_bundling_source(global)
            .set_finish_bundling_source(finish);
    }

    aggregation << task_aggregation;

    AST_t tree = src.parse_statement(construct.get_ast(),
            construct.get_scope_link());

    construct.get_ast().replace(tree);
}

static void simdize_loop_fun(TL::ForStatement& for_stmt,
			     TL::ObjectList<TL::IdExpression>* simd_id_exp_list = NULL)
{
    unsigned char min_stmt_size;

    TL::Expression lower_bound = for_stmt.get_lower_bound();

    //simdize_loop returns a Compound Statement!
    SIMDization * simdization = TL::HLT::simdize(for_stmt, min_stmt_size, simd_id_exp_list);
    TL::Source simdized_loop_src = *simdization; 
    delete(simdization);

    TL::AST_t simdized_loop_tree = simdized_loop_src.parse_statement(for_stmt.get_ast(),
            for_stmt.get_scope_link());

    for_stmt.get_ast().replace(simdized_loop_tree);



    TL::Statement& stmt = for_stmt;

    if (stmt.is_compound_statement())
    {
        TL::ObjectList<TL::Statement> statement_list = stmt.get_inner_statements();

        //SIMD for + Epilog for
        if (statement_list.size() != 2)
        {
            internal_error("The number of expected ForStatements is not correct.\n", 0);
        }

        TL::ForStatement& for_stmt_simd = (TL::ForStatement&) statement_list[0];
        TL::ForStatement& for_stmt_epilog = (TL::ForStatement&) statement_list[1];

        // This ForStatement is the unrolled loop (SIMD)
        for_stmt_simd.get_ast().set_attribute(LANG_HLT_SIMD_FOR_INFO, min_stmt_size);

        // This ForStatement is marked as Epilog
        TL::RefPtr<TL::Expression> lower_bound_ref(new TL::Expression(lower_bound));
        for_stmt_epilog.get_ast().set_attribute(LANG_HLT_SIMD_EPILOG, lower_bound_ref);
    }
    else
    {
        DEBUG_CODE()
        {
            std::cerr << for_stmt.get_ast().get_locus() 
                << ": warning: LoopSIMDization doesn't return a CompoundStatement." << std::endl;
        }
    }
}

static void simdize_function_fun(TL::FunctionDefinition& func_def)
{
    using namespace TL;
    using namespace TL::SIMD;
    
    unsigned char min_stmt_size;

    SIMDization * simdization = TL::HLT::simdize(func_def, min_stmt_size);
    TL::Source simdized_func_src = *simdization;
    delete(simdization);

    FunctionDefinition generic_func_def(simdized_func_src.parse_declaration(
            func_def.get_ast(), func_def.get_scope_link()),
            func_def.get_scope_link());

    generic_functions.add_generic_function(func_def.get_function_symbol(), 
            generic_func_def.get_function_symbol());
}


void HLTPragmaPhase::simdize(PragmaCustomConstruct construct)
{
    static bool first_time = true;
    //FIXME: SIMD pre run should be done in a new phase
    if (first_time)
    {
        simd_pre_run(construct.get_ast().get_enclosing_global_tree(),
                construct.get_scope_link());

        first_time = false;
    }

    //SIMD FUNCTIONS
    if (construct.is_function_definition())
    {
        
        FunctionDefinition func_def (construct.get_declaration(), construct.get_scope_link());
        if (construct.is_parameterized())
        {
            std::cerr << construct.get_ast().get_locus() 
                << ": warning: unexpected parameters in #pragma hlt simd" << std::endl;
        }

        simdize_function_fun(func_def);
        construct.get_ast().replace(func_def.get_ast());
        
    }
    //SIMD LOOPS
    else
    {
        Statement statement = construct.get_statement();

        if (ForStatement::predicate(statement.get_ast()))
        {
            ForStatement for_statement(statement.get_ast(), statement.get_scope_link());

            if (construct.is_parameterized())
            {
                ObjectList<IdExpression> simd_id_exp_list = construct.get_parameter_id_expressions();
                simdize_loop_fun(for_statement, &simd_id_exp_list);
            }
            else
            {
                simdize_loop_fun(for_statement);
            }

            construct.get_ast().replace(for_statement.get_ast());
        }
        else
        {
           std::cerr << construct.get_ast().get_locus() << ": warning: unexpected #pragma hlt simd" << std::endl;
        }
    }
}


EXPORT_PHASE(TL::HLT::HLTPragmaPhase)
