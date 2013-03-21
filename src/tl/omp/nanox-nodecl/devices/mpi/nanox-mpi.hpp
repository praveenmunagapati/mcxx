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


#ifndef NANOX_MPI_HPP
#define NANOX_MPI_HPP


#include "tl-compilerphase.hpp"
#include "tl-devices.hpp"

//This two vars MUST keep same value than the ones existing at NANOX
#define TAG_MAIN_OMPSS "__ompss_mpi_daemon" 
#define MASK_TASK_NUMBER 989

namespace TL
{

    namespace Nanox
    {

        class DeviceMPI : public DeviceProvider
        {
            private:

                  Nodecl::List _cuda_file_code;
                  bool _mpi_task_processed;
                  Source _sectionCodeHost;
                  Source _sectionCodeDevice;
                  Source _extraFortranDecls;
                  Nodecl::NodeclBase _root;
                  unsigned int _currTaskId;

                  void generate_additional_mpi_code(
                    const TL::ObjectList<Nodecl::NodeclBase>& onto_clause,
                    const TL::Symbol& struct_args,
                    const std::string& outline_name,
                    TL::Source& code_host,
                    TL::Source& code_device_pre,        
                    TL::Source& code_device_post);

//                  void add_included_cuda_files(FILE* file);                  
                  std::string get_ompss_mpi_type(Type type);
                  Nodecl::List _extra_c_code;

                  void add_forward_code_to_extra_c_code(
                    const std::string& outline_name,
                    TL::ObjectList<OutlineDataItem*> data_items,
                    Nodecl::NodeclBase parse_context);
                  
            public:

                // This phase does nothing
                virtual void pre_run(DTO& dto);
                virtual void run(DTO& dto);

                DeviceMPI();

                virtual ~DeviceMPI() { }

             virtual void create_outline(CreateOutlineInfo &info,
                     Nodecl::NodeclBase &outline_placeholder,
                     Nodecl::NodeclBase &output_statements,
                     Nodecl::Utils::SymbolMap* &symbol_map);

             virtual void get_device_descriptor(DeviceDescriptorInfo& info,
                     Source &ancillary_device_description,
                     Source &device_descriptor,
                     Source &fortran_dynamic_init);

             virtual bool remove_function_task_from_original_source() const;

             virtual void copy_stuff_to_device_file(
                     const TL::ObjectList<Nodecl::NodeclBase>& stuff_to_be_copied);

             bool allow_mandatory_creation();

              //  virtual void create_outline(
              //          const std::string& task_name,
              //          const std::string& struct_typename,
              //          DataEnvironInfo &data_environ,
              //          const OutlineFlags& outline_flags,
              //          AST_t reference_tree,
              //          ScopeLink sl,
              //          Source initial_setup,
              //          Source outline_body);

              //  virtual void do_replacements(DataEnvironInfo& data_environ,
              //          AST_t body,
              //          ScopeLink scope_link,
              //          Source &initial_setup,
              //          Source &replace_src);

              //  virtual void get_device_descriptor(const std::string& task_name,
              //          DataEnvironInfo &data_environ,
              //          const OutlineFlags& outline_flags,
              //          AST_t reference_tree,
              //          ScopeLink sl,
              //          Source &ancillary_device_description,
              //          Source &device_descriptor);

              //  virtual std::string get_outline_name_for_instrumentation(const std::string & name,
              //          bool is_template_specialized, const FunctionDefinition& enclosing_function) const;

               virtual void phase_cleanup(DTO& data_flow);

              //  virtual void insert_function_definition(PragmaCustomConstruct, bool);
              //  virtual void insert_declaration(PragmaCustomConstruct, bool);
        };

    }

}

#endif // NANOX_CUDA_HPP
