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



#ifndef TL_CFG_VISITOR_HPP
#define TL_CFG_VISITOR_HPP

#include <stack>

#include "tl-analysis-singleton.hpp"
#include "tl-extensible-graph.hpp"
#include "tl-nodecl-visitor.hpp"
#include "tl-node.hpp"


namespace TL
{
    namespace Analysis
    {
        struct loop_control_nodes_t {
            Node* init;
            Node* cond;
            Node* next;
            
            loop_control_nodes_t() 
                : init(NULL), cond(NULL), next(NULL)
            {}
            
            loop_control_nodes_t(const loop_control_nodes_t& loop_control)
            {
                init = loop_control.init;
                cond = loop_control.cond;
                next = loop_control.next;
            }
            
            void clear()
            {
                init = NULL;
                cond = NULL;
                next = NULL;
            }
            
            bool is_empty()
            {
                return ((init==NULL) && (cond==NULL) && (next==NULL));
            }
        };
        
        struct try_block_nodes_t {
            ObjectList<Node*> handler_parents;
            ObjectList<Node*> handler_exits;
            int nhandlers;
            
            try_block_nodes_t()
                : handler_parents(), handler_exits(), nhandlers(-1)
            {}
            
            try_block_nodes_t(const try_block_nodes_t& try_block)
            {
                handler_parents = try_block.handler_parents;
                handler_exits = try_block.handler_exits;
            }
            
            void clear()
            {
                handler_parents.clear();
                handler_exits.clear();
                nhandlers = -1;
            }  
        };
        
        struct clause_t {
            std::string clause;
            ObjectList<Nodecl::NodeclBase> args;
            
            clause_t()
                : clause(), args()
            {}
            
            clause_t(std::string s)
                : clause(s), args()
            {}
            
            clause_t(const clause_t& c)
            {
                clause = c.clause;
                args = c.args;
            }
        };    
        
        struct pragma_t {
            ObjectList<Nodecl::NodeclBase> params;
            ObjectList<struct clause_t> clauses;
            
            pragma_t()
                : params(), clauses()
            {}
            
            pragma_t(const pragma_t& p)
            {
                params = p.params;
                clauses = p.clauses;
            }
            
            bool has_clause(std::string s)
            {
                for (ObjectList<struct clause_t>::iterator it = clauses.begin();
                    it != clauses.end();
                    ++it)
                {
                    if (it->clause == s) return true;
                }
                
                return false;
            }
        };
        
        struct omp_pragma_sections_t {
            ObjectList<Node*> section_parents;
            ObjectList<Node*> sections_exits;
            
            omp_pragma_sections_t(ObjectList<Node*> parents)
                : section_parents(parents), sections_exits()
            {}
            
            omp_pragma_sections_t(const omp_pragma_sections_t& sections)
            {
                section_parents = sections.section_parents;
                sections_exits = sections.sections_exits;
            }
        };  
    
        class LIBTL_CLASS PCFGVisitor : public Nodecl::NodeclVisitor<TL::ObjectList<Node*> >
        {
        private:
            
            // *** All these variables are used while building of the graphs *** //
            
            ExtensibleGraph* _current_pcfg;
            
            std::stack<Nodecl::NodeclBase> _context_s;
            
            ObjectList<Node*> _return_nodes;
            
            std::stack<struct loop_control_nodes_t> _loop_info_s;
        
            ObjectList<struct try_block_nodes_t> _actual_try_info;
            
            std::stack<struct pragma_t> _pragma_info_s;
        
            std::stack<struct omp_pragma_sections_t> _omp_sections_info;
            
            std::stack<Node*> _switch_cond_s;
            
            ObjectList<Symbol> _visited_functions;
            
            //! This method creates a list with the nodes in an specific subgraph
            /*!
            * \param node First node to be traversed. The method will visit all nodes from here.
            */
            void compute_catch_parents(Node* node);
            
            //! This method merges a list of nodes containing an Expression into one
            /*!
            * The way the method merges the nodes depends on the kind of the nodes to be merged:
            * The nodes that are not a GRAPH NODE are deleted. The rest remain there to be the parents of the new node.
            * \param n Nodecl containing a Expression which will be wrapped in the new node
            * \param nodes_l List of nodes containing the different parts of an expression
            * \return The new node created
            */        
            Node* merge_nodes(Nodecl::NodeclBase n, ObjectList<Node*> nodes_l);
            
            //! This is a wrapper method of #merge_nodes for the case having only one or two nodes to be merged
            /*!
            * \param n Nodecl containing a Expression which will be wrapped in the new node
            * \param first Pointer to the node containing one part of the new node
            * \param second Pointer to the node containing other part of the new node
            */
            Node* merge_nodes(Nodecl::NodeclBase n, Node* first, Node* second);
            
            //! This a wrapper method of #merge_nodes for the case we are merging an array subscript
            /*!
            * Since the subscripts are built form left to right, we may not have a nodecl containing the whole subscript
            * \param subscripted Pointer to the node containing the subscripted part
            * \param subscript Pointer to the node containing the actual subscript
            */
            Node* merge_nodes(Node* subscripted, Node* subscript);
            
            //! This method finds the parent nodes in a sequence of connected nodes
            /*!
            * The method fails when the sub-graph has more than one entry node.
            * Since this method is used specifically to collapse the nodes created while building the node of an expression
            * we expect to find only one entry node. 
            * (We don't refer a node of BASIC_ENTRY_NODE type, but the first node in the sub-graph)
            * \param actual_node Node we are computing in this moment
            * \return The entry node of a sub-graph
            */
            ObjectList<Node*> get_first_nodes(Node* actual_node);
            
            //! This method implements the visitor for a CaseStatement and for DefaultStatement
            /*!
            * \param n Nodecl containing the Case or the Default Statement
            * \return The graph node created while the Statement has been parsed
            */
            template <typename T>
            Ret visit_Case_or_Default(const T& n);
            
            //! This method implements the visitor for a VirtualFunctionCall and a FunctionCall
            /*!
            * \param n Nodecl containinf the VirtualFunctionCall or the FunctionCall
            * \return The graph node created while the function call has been parsed
            */
            template <typename T>
            Ret function_call_visit(const T& n);
        
            //! This method build the graph node containing the CFG of a task
            /*!
            * The method stores the graph node into the list #_task_graphs_l
            * We can place the task any where in the graph taking into account that the position must 
            * respect the initial dependences
            */
            template <typename T>
            Ret create_task_graph(const T& n);
            
            template<typename T>
            Ret visit_pragma_construct(const T& n);
            
            
            // *** IPA *** //
                
            //! Computes the liveness information of each node regarding only its inner statements
            /*!
                A variable is Killed (X) when it is defined before than used in X.
                A variable is Upper Exposed (X) when it is used before than defined in X.
                */
            void gather_live_initial_information(Node* actual);
            
            //! Sets the initial liveness information of the node.
            /*!
            * The method computes the used and defined variables of a node taking into account only
            * the inner statements.
            */
            void set_live_initial_information(Node* node);
                
            bool propagate_use_rec(Node* actual);
            
            bool func_has_cyclic_calls_rec(Symbol reach_func, Symbol stop_func, ExtensibleGraph * graph);
            
            
        public:
            // *** Constructors *** //
            
            //! Default constructor
            /*!
            * This method is used when we want to perform the analysis from the Analyisis phase
            */
            PCFGVisitor();
            
            //! Constructor which built a CFG
            /*!
            * This method is used when we want to perform the analysis starting from any piece of code.
            * Not necessarily from a TopLevel or a FunctionCode node.
            */        
            PCFGVisitor(ExtensibleGraph* pcfg);
            
            
            // *** Non visiting methods *** //
            
            void build_pcfg(const Nodecl::NodeclBase& n);
            
            void set_actual_cfg(ExtensibleGraph* graph);

            void analyse_induction_variables( ExtensibleGraph* graph );

            
            // *** IPA *** //
            //! Computes the define-use chain of a node
            void compute_use_def_chains(Node* node);
            
            //! Analyse loops and ranged access to variables. Recomputes use-def and reaching definitions
            //! with the info of iterated accesses
            void analyse_loops(Node* node);
            
            //! Once the use-def chains are calculated for every graph, we are able to recalculate the use-def of every function call
            bool propagate_use_def_ipa(Node* node);        
            
            bool func_has_cyclic_calls(Symbol actual_func, ExtensibleGraph* graph);
            
            // *** Visiting methods *** //
            
            Ret unhandled_node(const Nodecl::NodeclBase& n);
            Ret visit(const Nodecl::Context& n);
            Ret visit(const Nodecl::TopLevel& n);
            Ret visit(const Nodecl::FunctionCode& n);
            Ret visit(const Nodecl::TryBlock& n);
            Ret visit(const Nodecl::CatchHandler& n);
            Ret visit(const Nodecl::Throw& n);
            Ret visit(const Nodecl::CompoundStatement& n);
            Ret visit(const Nodecl::Conversion& n);
            Ret visit(const Nodecl::Symbol& n);
            Ret visit(const Nodecl::ExpressionStatement& n);
            Ret visit(const Nodecl::ObjectInit& n);
            Ret visit(const Nodecl::ArraySubscript& n);
            Ret visit(const Nodecl::Range& n);
            Ret visit(const Nodecl::ClassMemberAccess& n);
            Ret visit(const Nodecl::FortranNamedPairSpec& n);
            Ret visit(const Nodecl::Concat& n);
            Ret visit(const Nodecl::New& n);
            Ret visit(const Nodecl::Delete& n);
            Ret visit(const Nodecl::DeleteArray& n);
            Ret visit(const Nodecl::Offsetof& n);
            Ret visit(const Nodecl::Sizeof& n);
            Ret visit(const Nodecl::Type& n);
            Ret visit(const Nodecl::Typeid& n);
            Ret visit(const Nodecl::Cast& n);
            Ret visit(const Nodecl::Alignof& n);
            Ret visit(const Nodecl::Offset& n);
            Ret visit(const Nodecl::VirtualFunctionCall& n);
            Ret visit(const Nodecl::FunctionCall& n);
            Ret visit(const Nodecl::StringLiteral& n);
            Ret visit(const Nodecl::BooleanLiteral& n);
            Ret visit(const Nodecl::IntegerLiteral& n);
            Ret visit(const Nodecl::ComplexLiteral& n);
            Ret visit(const Nodecl::FloatingLiteral& n);
            Ret visit(const Nodecl::StructuredValue& n);
            Ret visit(const Nodecl::EmptyStatement& n);
            Ret visit(const Nodecl::ReturnStatement& n);
            Ret visit(const Nodecl::PragmaCustomDirective& n);
            Ret visit(const Nodecl::PragmaCustomStatement& n);
            Ret visit(const Nodecl::PragmaCustomDeclaration& n);
            Ret visit(const Nodecl::PragmaCustomClause& n);
            Ret visit(const Nodecl::PragmaCustomLine& n);
            Ret visit(const Nodecl::PragmaClauseArg& n);
            Ret visit(const Nodecl::ForStatement& n);
            Ret visit(const Nodecl::WhileStatement& n);
            Ret visit(const Nodecl::IfElseStatement& n);
            Ret visit(const Nodecl::SwitchStatement& n);
            Ret visit(const Nodecl::CaseStatement& n);
            Ret visit(const Nodecl::DefaultStatement& n);
            Ret visit(const Nodecl::ConditionalExpression& n);
            Ret visit(const Nodecl::FortranComputedGotoStatement& n);
            Ret visit(const Nodecl::FortranAssignedGotoStatement& n);
            Ret visit(const Nodecl::GotoStatement& n);
            Ret visit(const Nodecl::LabeledStatement& n);
            Ret visit(const Nodecl::LoopControl& n);
            Ret visit(const Nodecl::ContinueStatement& n);
            Ret visit(const Nodecl::BreakStatement& n);
            Ret visit(const Nodecl::DoStatement& n);
            Ret visit(const Nodecl::Assignment& n);
            Ret visit(const Nodecl::AddAssignment& n);
            Ret visit(const Nodecl::MinusAssignment& n);
            Ret visit(const Nodecl::DivAssignment& n);
            Ret visit(const Nodecl::MulAssignment& n);
            Ret visit(const Nodecl::ModAssignment& n);
            Ret visit(const Nodecl::BitwiseAndAssignment& n);
            Ret visit(const Nodecl::BitwiseOrAssignment& n);
            Ret visit(const Nodecl::BitwiseXorAssignment& n);
            Ret visit(const Nodecl::BitwiseShrAssignment& n);
            Ret visit(const Nodecl::ArithmeticShrAssignment& n);
            Ret visit(const Nodecl::BitwiseShlAssignment& n);
            Ret visit(const Nodecl::Add& n);
            Ret visit(const Nodecl::Minus& n);
            Ret visit(const Nodecl::Mul& n);
            Ret visit(const Nodecl::Div& n);
            Ret visit(const Nodecl::Mod& n);
            Ret visit(const Nodecl::Power& n);
            Ret visit(const Nodecl::LogicalAnd& n);
            Ret visit(const Nodecl::LogicalOr& n);
            Ret visit(const Nodecl::LogicalNot& n);
            Ret visit(const Nodecl::BitwiseAnd& n);
            Ret visit(const Nodecl::BitwiseOr& n);
            Ret visit(const Nodecl::BitwiseXor& n);
            Ret visit(const Nodecl::BitwiseNot& n);
            Ret visit(const Nodecl::Equal& n);
            Ret visit(const Nodecl::Different& n);
            Ret visit(const Nodecl::LowerThan& n);
            Ret visit(const Nodecl::GreaterThan& n);
            Ret visit(const Nodecl::LowerOrEqualThan& n);
            Ret visit(const Nodecl::GreaterOrEqualThan& n);
            Ret visit(const Nodecl::ArithmeticShr& n);
            Ret visit(const Nodecl::BitwiseShr& n);
            Ret visit(const Nodecl::BitwiseShl& n);
            Ret visit(const Nodecl::Predecrement& n);
            Ret visit(const Nodecl::Postdecrement& n);
            Ret visit(const Nodecl::Preincrement& n);
            Ret visit(const Nodecl::Postincrement& n);
            Ret visit(const Nodecl::Plus& n);
            Ret visit(const Nodecl::Neg& n);     
            Ret visit(const Nodecl::Dereference& n);
            Ret visit(const Nodecl::Reference& n);
            Ret visit(const Nodecl::Text& n);
            Ret visit(const Nodecl::FortranWhere& n);
            Ret visit(const Nodecl::FortranWherePair& n);
            Ret visit(const Nodecl::FortranLabelAssignStatement& n);
            Ret visit(const Nodecl::FortranIoSpec& n);
            Ret visit(const Nodecl::FieldDesignator& n);
            Ret visit(const Nodecl::IndexDesignator& n);
            Ret visit(const Nodecl::FortranEquivalence& n);
            Ret visit(const Nodecl::FortranData& n);
            Ret visit(const Nodecl::FortranImpliedDo& n);
            Ret visit(const Nodecl::FortranForall& n);    
            Ret visit(const Nodecl::FortranArithmeticIfStatement& n);
            Ret visit(const Nodecl::FortranNullifyStatement& n);
            Ret visit(const Nodecl::FortranIoStatement& n);  
            Ret visit(const Nodecl::FortranOpenStatement& n);
            Ret visit(const Nodecl::FortranCloseStatement& n);
            Ret visit(const Nodecl::FortranReadStatement& n);
            Ret visit(const Nodecl::FortranWriteStatement& n);
            Ret visit(const Nodecl::FortranPrintStatement& n);
            Ret visit(const Nodecl::FortranStopStatement& n);
            Ret visit(const Nodecl::FortranAllocateStatement& n);
            Ret visit(const Nodecl::FortranDeallocateStatement& n);
            Ret visit(const Nodecl::Comma& n);
        };
    }
}

#endif  // TL_CFG_VISITOR_HPP