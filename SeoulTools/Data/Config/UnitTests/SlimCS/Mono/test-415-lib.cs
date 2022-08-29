// Compiler options: -t:library

using System;
public abstract class MyTestAbstract
{
	protected abstract string GetName();
	
	public MyTestAbstract()
	{
	}

	public void PrintName()
	{
		SlimCS.print("Name=" + GetName());
	}
}
