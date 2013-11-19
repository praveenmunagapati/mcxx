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



/*
<testinfo>
test_generator="config/mercurium run"
</testinfo>
*/

template <typename T>
struct A
{
    T t;
};

template <typename T>
struct B
{
    static A<T*> a;
};

template <typename T>
A<T*> B<T>::a;

template A<int*> B<int>::a;

extern "C"
{
    // This is the mangling of B<int>::a
    // just to ensure B<int>::a has been emitted
    extern int _ZN1BIiE1aE;
}

int main(int argc, char **argv)
{
    _ZN1BIiE1aE = 3;

    return 0;
}
