using static SlimCS;
public static class Test
{
	public delegate void FunctionDelegate();
	public delegate void ParemeterFunctionDelegate(double param);
	
	private class MemberWithDelegate
	{
		private double m_fMemberDouble = 123.45;
		
		private void PrintMember()
		{
			print(m_fMemberDouble);
		}
		
		private void PrintParemeterAndMember(double param)
		{
			print(m_fMemberDouble);
			print(param);
		}
		
		public FunctionDelegate GetFunction()
		{
			return PrintMember;
		}
		
		public ParemeterFunctionDelegate GetParemeterFunction()
		{
			return PrintParemeterAndMember;
		}
	}

	public static int Main ()
	{
		MemberWithDelegate member = new MemberWithDelegate();
		
		FunctionDelegate func = (FunctionDelegate)member.GetFunction();
		ParemeterFunctionDelegate paramfunc = (ParemeterFunctionDelegate)member.GetParemeterFunction();
		
		func();
		paramfunc(67.89);
		
		return 0;
	}
}
