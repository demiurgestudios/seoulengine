//
// fixed
//
using System;

class X {

	void A ()
	{
	}
				
	public static void Main ()
	{
		int loop = 0;
		
		goto a;
	b:
		loop++;
		return;
	a:
		SlimCS.print ("Hello");
		for (;;){
			goto b;
		}
	}
}
