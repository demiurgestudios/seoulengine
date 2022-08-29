using System;

public class ExceptionWithAnonMethod
{
	public delegate void EmptyCallback();
    	static string res;
	
	public static int Main()
	{
		try {
			throw new Exception("e is afraid to enter anonymous land");
		} catch(Exception e) {
			AnonHandler(delegate {
				SlimCS.print(e.Message); 
				res = e.Message;
			});
		}
		if (res == "e is afraid to enter anonymous land"){
		    SlimCS.print ("Test passed");
		    return 0;
		}
		SlimCS.print ("Test failed");
		return 1;
	}

	public static void AnonHandler(EmptyCallback handler)
	{
		if(handler != null) {
			handler();
		}
	}
}
