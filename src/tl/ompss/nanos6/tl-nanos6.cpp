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


#include "tl-nanos6.hpp"
#include "tl-nanos6-lower.hpp"

namespace TL { namespace Nanos6 {

    LoweringPhase::LoweringPhase()
    {
        set_phase_name("Nanos 6 lowering");
        set_phase_description("This phase lowers from Mercurium parallel IR "
                "into real code involving the Nanos 6 runtime interface");

        // std::cerr << "Initializing Nanos 6 lowering phase" << std::endl;
    }

    void LoweringPhase::run(DTO& dto)
    {
        std::cerr << "Nanos 6 phase" << std::endl;

        FORTRAN_LANGUAGE()
        {
            this->fortran_load_api(dto);
        }

        Nodecl::NodeclBase translation_unit =
            *std::static_pointer_cast<Nodecl::NodeclBase>(dto["nodecl"]);

        Lower lower(this);
        lower.walk(translation_unit);
    }

    void LoweringPhase::pre_run(DTO& dto)
    {
        std::cerr << "Nanos 6 prerun" << std::endl;
    }

    void LoweringPhase::phase_cleanup(DTO& dto)
    {
        // std::cerr << "Nanos 6 phase cleanup" << std::endl;
    }

} }

EXPORT_PHASE(TL::Nanos6::LoweringPhase);
