/*--------------------------------------------------------------------
  (C) Copyright 2015-2015 Barcelona Supercomputing Center
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


#include "tl-nanos6-lower.hpp"
#include "tl-nanos6-task-properties.hpp"
#include "tl-counters.hpp"
#include "tl-source.hpp"

#include "cxx-exprtype.h"

namespace TL { namespace Nanos6 {

    void Lower::visit(const Nodecl::OpenMP::Task& node)
    {
        Nodecl::NodeclBase stmts = node.get_statements();
        walk(stmts);

        TaskProperties task_properties = TaskProperties::gather_task_properties(_phase, node);

        Nodecl::NodeclBase args_size;
        TL::Type data_env_struct;
        task_properties.create_environment_structure(
                /* out */
                data_env_struct,
                args_size);

        TL::Symbol task_info, task_invocation_info;
        Nodecl::NodeclBase local_init_task_info;
        task_properties.create_task_info(
                /* out */
                task_info,
                task_invocation_info,
                local_init_task_info);

        TL::Scope sc = node.retrieve_context();

        std::string args_name;
        {
            TL::Counter &counter = TL::CounterManager::get_counter("nanos6-task-args");
            std::stringstream ss;
            ss << "nanos_data_env_" << (int)counter;
            counter++;
            args_name = ss.str();
        }

        TL::Symbol args = sc.new_symbol(args_name);
        args.get_internal_symbol()->kind = SK_VARIABLE;
        args.set_type(data_env_struct.get_pointer_to());
        symbol_entity_specs_set_is_user_declared(
                args.get_internal_symbol(),
                1);

        std::string task_ptr_name;
        {
            TL::Counter &counter = TL::CounterManager::get_counter("nanos6-task-ptr");
            std::stringstream ss;
            ss << "nanos_task_ptr_" << (int)counter;
            counter++;
            task_ptr_name = ss.str();
        }
        TL::Symbol task_ptr = sc.new_symbol(task_ptr_name);
        task_ptr.get_internal_symbol()->kind = SK_VARIABLE;
        task_ptr.set_type(TL::Type::get_void_type().get_pointer_to());
        symbol_entity_specs_set_is_user_declared(
                task_ptr.get_internal_symbol(), 1);

        Nodecl::List new_stmts;
        if (!local_init_task_info.is_null())
        {
            // Init task info if it happens to be local
            new_stmts.append(local_init_task_info);
        }

        // Create task
        {
            if (IS_CXX_LANGUAGE)
            {
                new_stmts.append(
                        Nodecl::CxxDef::make(Nodecl::NodeclBase::null(), args));
                new_stmts.append(
                        Nodecl::CxxDef::make(Nodecl::NodeclBase::null(), task_ptr));
            }

            TL::Symbol nanos_create_task_sym =
                TL::Scope::get_global_scope().get_symbol_from_name("nanos_create_task");
            ERROR_CONDITION(!nanos_create_task_sym.is_valid()
                    || !nanos_create_task_sym.is_function(),
                    "Invalid symbol", 0);

            // Source new_task_src;
            // new_task_src
            //     << "nanos_create_task("
            //     <<    "&" << as_symbol(task_info) << ","
            //     <<    as_expression(args_size) << ","
            //     /* out */
            //     <<    "(void**)&" << as_symbol(args) << ","
            //     <<    "&" << as_symbol(task_ptr) << ");"
            //     ;

            // &task_info
            Nodecl::NodeclBase task_info_ptr =
                Nodecl::Reference::make(
                    task_info.make_nodecl(
                        /* set_ref_type */ true,
                        node.get_locus()),
                    task_info.get_type().get_pointer_to(),
                    node.get_locus());

            // (void**)&args
            Nodecl::NodeclBase args_ptr_out =
                Nodecl::Cast::make(
                        Nodecl::Reference::make(
                            args.make_nodecl(
                                /* set_ref_type */ true,
                                node.get_locus()),
                            args.get_type().get_pointer_to(),
                            node.get_locus()),
                        TL::Type::get_void_type().get_pointer_to().get_pointer_to(),
                        "C",
                        node.get_locus());

            // &task_ptr
            Nodecl::NodeclBase task_ptr_out =
                Nodecl::Reference::make(
                        task_ptr.make_nodecl(
                            /* set_ref_type */ true,
                            node.get_locus()),
                        task_ptr.get_type().get_pointer_to(),
                        node.get_locus());

            Nodecl::NodeclBase task_invocation_info_ptr =
                Nodecl::Reference::make(
                        task_invocation_info.make_nodecl(
                            /* set_ref_type */ true,
                            node.get_locus()),
                        task_invocation_info.get_type().get_pointer_to(),
                        node.get_locus());

            Nodecl::NodeclBase call_to_nanos_create_task =
                Nodecl::ExpressionStatement::make(
                        Nodecl::FunctionCall::make(
                            nanos_create_task_sym.make_nodecl(/* set_ref_type */ true,
                                node.get_locus()),
                            Nodecl::List::make(
                                task_info_ptr,
                                task_invocation_info_ptr,
                                args_size,
                                /* out */
                                args_ptr_out,
                                task_ptr_out),
                            /* alternate symbol */ Nodecl::NodeclBase::null(),
                            /* function form */ Nodecl::NodeclBase::null(),
                            TL::Type::get_void_type(),
                            node.get_locus()),
                        node.get_locus());

            new_stmts.append(call_to_nanos_create_task);
        }

        // Capture environment
        {
            Nodecl::NodeclBase capture_env;
            task_properties.capture_environment(
                    args,
                    /* out */ capture_env);

            new_stmts.append(capture_env);
        }

        // Submit the created task
        {
            TL::Symbol nanos_submit_task_sym =
                TL::Scope::get_global_scope().get_symbol_from_name("nanos_submit_task");
            ERROR_CONDITION(!nanos_submit_task_sym.is_valid()
                    || !nanos_submit_task_sym.is_function(),
                    "Invalid symbol", 0);

            Nodecl::NodeclBase task_ptr_arg = task_ptr.make_nodecl(/* set_ref_type */ true);
            task_ptr_arg = ::cxx_nodecl_make_conversion(
                    task_ptr_arg.get_internal_nodecl(),
                    task_ptr.get_type().no_ref().get_internal_type(),
                    node.get_locus());

            Nodecl::NodeclBase new_task =
                Nodecl::ExpressionStatement::make(
                        Nodecl::FunctionCall::make(
                            nanos_submit_task_sym.make_nodecl(/* set_ref_type */ true),
                            Nodecl::List::make(task_ptr_arg),
                            /* alternate symbol */ Nodecl::NodeclBase::null(),
                            /* function form */ Nodecl::NodeclBase::null(),
                            TL::Type::get_void_type(),
                            node.get_locus()),
                        node.get_locus());

            new_stmts.append(new_task);
        }

        node.replace(new_stmts);
    }

} }