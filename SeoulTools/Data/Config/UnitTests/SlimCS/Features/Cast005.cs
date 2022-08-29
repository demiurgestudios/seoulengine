using static SlimCS;
using System;

class Test
{
	class A {}
	class B {}
	class C {}

	public static void Main ()
	{
		object o;
		o = new A();
		try { print((A)o); } catch (System.Exception e) { print(e.Message); }
		try { print((B)o); } catch (System.Exception e) { print("Cast failed"); }
		try { print((C)o); } catch (System.Exception e) { print("Cast failed"); }

		o = new B();
		try { print((A)o); } catch (System.Exception e) { print("Cast failed"); }
		try { print((B)o); } catch (System.Exception e) { print(e.Message); }
		try { print((C)o); } catch (System.Exception e) { print("Cast failed"); }

		o = new C();
		try { print((A)o); } catch (System.Exception e) { print("Cast failed"); }
		try { print((B)o); } catch (System.Exception e) { print("Cast failed"); }
		try { print((C)o); } catch (System.Exception e) { print(e.Message); }

		o = true;
		try { print((A)o); } catch (System.Exception e) { print("Cast failed"); }
		try { print((B)o); } catch (System.Exception e) { print("Cast failed"); }
		try { print((C)o); } catch (System.Exception e) { print("Cast failed"); }

		o = 1.0;
		try { print((A)o); } catch (System.Exception e) { print("Cast failed"); }
		try { print((B)o); } catch (System.Exception e) { print("Cast failed"); }
		try { print((C)o); } catch (System.Exception e) { print("Cast failed"); }

		o = "Hello World";
		try { print((A)o); } catch (System.Exception e) { print("Cast failed"); }
		try { print((B)o); } catch (System.Exception e) { print("Cast failed"); }
		try { print((C)o); } catch (System.Exception e) { print("Cast failed"); }
	}
}
