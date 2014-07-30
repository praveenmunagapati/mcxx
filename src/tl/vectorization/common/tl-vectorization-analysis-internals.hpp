/*--------------------------------------------------------------------
  (C) Copyright 2006-2012 Barcelona Supercomputing Center
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

#ifndef TL_VECTORIZATION_ANALYSIS_INTERNALS_HPP
#define TL_VECTORIZATION_ANALYSIS_INTERNALS_HPP

#include "tl-vectorization-common.hpp"
#include "tl-node.hpp"
#include "tl-extensible-graph.hpp"

#include <set>

namespace TL
{
namespace Vectorization
{
    bool is_adjacent_access_internal(
            Analysis::Node* const scope_node,
            Analysis::Node* const n_node,
            const Nodecl::NodeclBase& n,
            Analysis::ExtensibleGraph* const pcfg,
            std::set<Nodecl::NodeclBase> visited_nodes =
        std::set<Nodecl::NodeclBase>());

    bool is_simd_aligned_access_internal(
            const Nodecl::NodeclBase& scope,
            const Nodecl::NodeclBase& n,
            const tl_sym_int_map_t& aligned_expressions,
            const objlist_nodecl_t& suitable_expressions,
            int unroll_factor, int alignment);

     bool is_suitable_expression_internal(
            const Nodecl::NodeclBase& scope, const Nodecl::NodeclBase& n,
            const objlist_nodecl_t& suitable_expressions, 
            int unroll_factor, int alignment, int& vector_size_module);
}
}

#endif //TL_VECTORIZATION_ANALYSIS_INTERNALS_HPP

