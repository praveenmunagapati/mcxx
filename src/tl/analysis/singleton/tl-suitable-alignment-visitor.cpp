/*--------------------------------------------------------------------
  (C) Copyright 2006-2013 Barcelona Supercomputing Center
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

#include "tl-suitable-alignment-visitor.hpp"

#include "cxx-cexpr.h"
#include "tl-analysis-utils.hpp"
//#include "tl-analysis-static-info.hpp"
//#include "tl-expression-reduction.hpp"
//#include <algorithm>

namespace TL
{
namespace Analysis
{
    SuitableAlignmentVisitor::SuitableAlignmentVisitor( Node* scope,
            const ObjectList<Nodecl::NodeclBase>& suitable_expressions, int unroll_factor,
            int type_size, int alignment )
        : _induction_variables( scope->get_induction_variables() ), _suitable_expressions( suitable_expressions ),
        _unroll_factor( unroll_factor ), _type_size( type_size ), _alignment( alignment )
    {
    }

    int SuitableAlignmentVisitor::join_list( ObjectList<int>& list )
    {
        int result = 0;
        for( ObjectList<int>::iterator it = list.begin( ); it != list.end( ); ++it )
        {
            result = result + ( *it );
        }
        return result;
    }

    bool SuitableAlignmentVisitor::is_aligned_access( const Nodecl::ArraySubscript& n,
            const std::map<TL::Symbol, int> aligned_expressions)
    {
        int i;
        int alignment;

        Nodecl::NodeclBase subscripted = n.get_subscripted( );
        TL::Type element_type = subscripted.get_type( );

        subscripted = Nodecl::Utils::advance_conversions(subscripted);

        // Linearized multidimensional arrays
        if (subscripted.is<Nodecl::Cast>())
        {
            subscripted = Nodecl::Utils::advance_conversions(
                    subscripted.as<Nodecl::Cast>().get_rhs());
        }

        ERROR_CONDITION(!subscripted.is<Nodecl::Symbol>(), "Subscripted is not a Nodecl::Symbol", 0);

        std::map<TL::Symbol, int>::const_iterator alignment_info = aligned_expressions.find(
                subscripted.as<Nodecl::Symbol>().get_symbol());

        if(alignment_info == aligned_expressions.end())
        {
            // There is no alignment info about the subscripted symbol
            // Assume unaligned
            return false;
        }
        else
        {
            // Get the alignment info of subscripted symbol
            alignment = alignment_info->second;
        }

        Nodecl::List subscripts = n.get_subscripts( ).as<Nodecl::List>( );
        int num_subscripts = subscripts.size( );

        // Get dimension sizes
        int *dimension_sizes = (int *)malloc( ( num_subscripts-1 ) * sizeof( int ) );

        for( i = 0; i < (num_subscripts-1); i++ ) // Skip the first one. It does not have size
        {
            // Iterate on array subscript type
            if( element_type.is_array( ) )
            {
                element_type = element_type.array_element( );
            }
            else if( element_type.is_pointer( ) )
            {
                element_type = element_type.points_to( );
            }
            else
            {
                WARNING_MESSAGE( "Array subscript does not have array type or pointer to array type", 0 );
                free( dimension_sizes );
                return false;
            }

            if( !element_type.array_has_size( ) )
            {
                WARNING_MESSAGE( "Array type does not have size", 0 );
                free( dimension_sizes );
                return false;
            }

            // Compute dimension alignment
            Nodecl::NodeclBase dimension_size_node = element_type.array_get_size( );

            // If VLA, get the actual size
            if(dimension_size_node.is<Nodecl::Symbol>() &&
                    dimension_size_node.get_symbol().is_saved_expression())
            {
                dimension_size_node = dimension_size_node.get_symbol().get_value();
            }

            int dimension_size = -1;
            if( dimension_size_node.is_constant( ) )
            {
                dimension_size = const_value_cast_to_signed_int( dimension_size_node.get_constant( ) ) * _type_size;
            }
            // If dimension size is suitable
            else if( is_suitable_expression( dimension_size_node ) )
            {
                dimension_size = _alignment;
            }
            if( VERBOSE )
                printf( "Dim %d, size %d\n", i, dimension_size );

            dimension_sizes[i] = dimension_size;
        }

        int it_alignment = -1;
        Nodecl::List::iterator it = subscripts.begin( );
        // Multiply dimension sizes by indexes
        for( i=0; it != subscripts.end( ); i++ )
        {
            it_alignment = walk( *it );

            it++;
            if( it == subscripts.end( ) ) break; // Last dimmension does not have to be multiplied

            // a[i][j][k] -> i -> i*J*K
            for( int j = i; j < (num_subscripts-1); j++ )
            {
                /*
                   if( ( is_suitable_constant( dimension_sizes[j] ) ) || is_suitable_constant( it_alignment ) )
                   {
                   it_alignment = 0;
                   }
                   else
                 */
                //                    if( ( dimension_sizes[j] == -1 ) || ( it_alignment == -1 ) )
                if( ( dimension_sizes[j] != -1 ) )
                {
                    if (it_alignment == -1)
                        it_alignment = dimension_sizes[j];
                    else
                        it_alignment *= dimension_sizes[j];
                }
                else
                {
                    it_alignment = -1;
                }
            }

            if( it_alignment == -1 )
            {
                free( dimension_sizes );
                return false;
            }

            alignment += it_alignment;
        }

        if( it_alignment == -1 )
        {
            free( dimension_sizes );
            return false;
        }

        // Add adjacent dimension
        alignment += it_alignment;

        free( dimension_sizes );

        if( (alignment % _alignment) == 0 )
            return true;

        return false;
    }

    bool SuitableAlignmentVisitor::is_suitable_expression( Nodecl::NodeclBase n )
    {
        bool result = true;
        if( !Nodecl::Utils::list_contains_nodecl( _suitable_expressions, n ) )
            result = false;
        return result;
    }

    bool SuitableAlignmentVisitor::is_suitable_constant( int n )
    {
        if ( (n % _alignment) == 0 )
            return true;
        else
            return false;
    }

    int SuitableAlignmentVisitor::visit( const Nodecl::Add& n )
    {
        if (is_suitable_expression(n))
        {
            return _alignment;
        }

        int lhs_mod = walk( n.get_lhs( ) );
        int rhs_mod = walk( n.get_rhs( ) );

        if( ( lhs_mod != -1 ) && ( rhs_mod != -1 ) )
            return lhs_mod + rhs_mod;

        return -1;
    }

    int SuitableAlignmentVisitor::visit( const Nodecl::ArraySubscript& n )
    {
        if (is_suitable_expression(n))
        {
            return _alignment;
        }

        return -1;
    }

    int SuitableAlignmentVisitor::visit( const Nodecl::BitwiseShl& n )
    {
        if (is_suitable_expression(n))
        {
            return _alignment;
        }

        int lhs_mod = walk( n.get_lhs( ) );
        int rhs_mod = walk( n.get_rhs( ) );

        // Something suitable multiplied by anything is suitable
        if (rhs_mod > 0)
        {
            // Because a << const is: a * (1 << const)
            if( (is_suitable_constant(lhs_mod)) || (is_suitable_constant(1 << rhs_mod) ))
                return 0;
            else if( ( lhs_mod != -1 ) && ( rhs_mod != -1 ) )
                return lhs_mod << rhs_mod;
        }

        return -1;
    }

    int SuitableAlignmentVisitor::visit( const Nodecl::BitwiseShr& n )
    {
        if (is_suitable_expression(n))
        {
            return _alignment;
        }

        int lhs_mod = walk( n.get_lhs( ) );
        int rhs_mod = walk( n.get_rhs( ) );

        // Something suitable multiplied by anything is suitable
        if (rhs_mod > 0)
        {
            // Because a << const is: a / (1 << const)
            if( (is_suitable_constant(lhs_mod)) || (is_suitable_constant(1 << rhs_mod) ))
                return 0;
            else if( ( lhs_mod > 0 ) && ( rhs_mod > 0 ) )
                return lhs_mod >> rhs_mod;
        }

        return -1;
    }

    int SuitableAlignmentVisitor::visit( const Nodecl::Conversion& n )
    {
        if (is_suitable_expression(n))
        {
            return _alignment;
        }

        return walk(n.get_nest());
    }

    int SuitableAlignmentVisitor::visit( const Nodecl::IntegerLiteral& n )
    {
        return const_value_cast_to_signed_int( n.get_constant( ) ) * _type_size;
    }

    int SuitableAlignmentVisitor::visit( const Nodecl::Neg& n )
    {
        if (is_suitable_expression(n))
        {
            return _alignment;
        }

        int rhs_mod = walk( n.get_rhs( ) );

        if( rhs_mod != -1 )
        {
            int result = -rhs_mod;
            if (result < 0)
                result = _alignment + result;

            return result;
        }

        return -1;
    }

    int SuitableAlignmentVisitor::visit( const Nodecl::Minus& n )
    {
        if (is_suitable_expression(n))
        {
            return _alignment;
        }

        int lhs_mod = walk( n.get_lhs( ) );
        int rhs_mod = walk( n.get_rhs( ) );

        if( ( lhs_mod != -1 ) && ( rhs_mod != -1 ) )
        {
            return lhs_mod - rhs_mod;
        }

        return -1;
    }

    int SuitableAlignmentVisitor::visit( const Nodecl::Mul& n )
    {
        if (is_suitable_expression(n))
        {
            return _alignment;
        }

        int lhs_mod = walk( n.get_lhs( ) );
        int rhs_mod = walk( n.get_rhs( ) );

        // Something suitable multiplied by anything is suitable
        if( (is_suitable_constant(lhs_mod)) || (is_suitable_constant(rhs_mod) ))
            return _alignment;
        else if( ( lhs_mod != -1 ) && ( rhs_mod != -1 ) )
            return lhs_mod * rhs_mod;

        return -1;
    }

    int SuitableAlignmentVisitor::visit( const Nodecl::ParenthesizedExpression& n )
    {
        if (is_suitable_expression(n))
        {
            return _alignment;
        }

        return walk(n.get_nest());
    }

    int SuitableAlignmentVisitor::visit( const Nodecl::Symbol& n )
    {
        if (is_suitable_expression(n))
        {
            return _alignment;
        }
        else if( n.is_constant( ) )
        {
            return const_value_cast_to_signed_int( n.get_constant( )) * _type_size;
        }
        else if( Utils::induction_variable_list_contains_variable( _induction_variables, n ) )
        {
            Utils::InductionVariableData* iv = Utils::get_induction_variable_from_list( _induction_variables, n );
            //Optimizations::ReduceExpressionVisitor v;

            Nodecl::NodeclBase lb = iv->get_lb(); //.shallow_copy( );
            Nodecl::NodeclBase incr = iv->get_increment();

            int lb_mod = walk(lb);
            int incr_mod = walk(incr);

            if (lb_mod != -1 && incr_mod != -1)
                return lb_mod + incr_mod * _unroll_factor;
        }

        return -1;
    }

    int SuitableAlignmentVisitor::unhandled_node(const Nodecl::NodeclBase& n)
    {
        WARNING_MESSAGE( "Suitable Alignment Visitor: Unknown node '%s' at '%s'\n",
                ast_print_node_type( n.get_kind( ) ), n.get_locus_str( ).c_str( ) );
        return -1;
    }
}
}
