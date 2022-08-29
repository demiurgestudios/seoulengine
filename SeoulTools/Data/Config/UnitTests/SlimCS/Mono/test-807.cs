using System;

class AException : Exception
{
}

class Program
{
	public static int Main ()
	{
		try {
			throw new AException ();
		} catch (AException e1) {
			SlimCS.print ("a");
			try {
			} catch (Exception) {
			}
			
			return 0;
		} catch (Exception e) {
			SlimCS.print ("e");
		}
		
		return 1;
	}
}