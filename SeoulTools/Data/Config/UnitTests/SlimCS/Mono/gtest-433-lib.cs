// Compiler options: -target:library

using System.Runtime.CompilerServices;
using System;

[assembly:InternalsVisibleTo("gtest-433")]

namespace Blah {

// internal by default
class Class1 
{
    public void Test() 
    {
        SlimCS.print("Class1.Test");
    }
}

// public type with internal member
public class Class2 
{
    internal void Test() 
    {
        SlimCS.print("Class2.Test");
    }

    internal enum Citrus {
        Lemon,
        Lime,
        Orange
    }
}

}
