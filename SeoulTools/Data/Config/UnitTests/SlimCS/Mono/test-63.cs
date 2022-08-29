//
// Tests rethrowing an exception
//
using System;

class X {
	public static int Main ()
	{
		bool one = false, two = false;

		try {
			try {
				throw new Exception ();
			} catch (Exception e) {
				one = true;
				SlimCS.print ("Caught");
				throw;
			} 
		} catch {
			two = true;
			SlimCS.print ("Again");
		}
		
		if (one && two){
			SlimCS.print ("Ok");
			return 0;
		} else
			SlimCS.print ("Failed");
		return 1;
	}
}
