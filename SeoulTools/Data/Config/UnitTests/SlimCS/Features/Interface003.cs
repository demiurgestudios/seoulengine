using static SlimCS;
static class Test
{
	interface A {}
	interface B {}
	interface C : A {}
	interface D : B {}
	interface E : A, B {}
	interface F : A, B, C, D, E {}

	class CA : A {}
	class CB : B {}
	class CC : C {}
	class CD : D {}
	class CE : E {}
	class CF : F {}

	static void Main()
	{
		object o = null;
		o = new CA();
		try { print((A)o); } catch (System.Exception e) { print("Cast failed."); }
		try { print((B)o); } catch (System.Exception e) { print("Cast failed."); }
		try { print((C)o); } catch (System.Exception e) { print("Cast failed."); }
		try { print((D)o); } catch (System.Exception e) { print("Cast failed."); }
		try { print((E)o); } catch (System.Exception e) { print("Cast failed."); }
		try { print((F)o); } catch (System.Exception e) { print("Cast failed."); }

		o = new CB();
		try { print((A)o); } catch (System.Exception e) { print("Cast failed."); }
		try { print((B)o); } catch (System.Exception e) { print("Cast failed."); }
		try { print((C)o); } catch (System.Exception e) { print("Cast failed."); }
		try { print((D)o); } catch (System.Exception e) { print("Cast failed."); }
		try { print((E)o); } catch (System.Exception e) { print("Cast failed."); }
		try { print((F)o); } catch (System.Exception e) { print("Cast failed."); }

		o = new CC();
		try { print((A)o); } catch (System.Exception e) { print("Cast failed."); }
		try { print((B)o); } catch (System.Exception e) { print("Cast failed."); }
		try { print((C)o); } catch (System.Exception e) { print("Cast failed."); }
		try { print((D)o); } catch (System.Exception e) { print("Cast failed."); }
		try { print((E)o); } catch (System.Exception e) { print("Cast failed."); }
		try { print((F)o); } catch (System.Exception e) { print("Cast failed."); }

		o = new CD();
		try { print((A)o); } catch (System.Exception e) { print("Cast failed."); }
		try { print((B)o); } catch (System.Exception e) { print("Cast failed."); }
		try { print((C)o); } catch (System.Exception e) { print("Cast failed."); }
		try { print((D)o); } catch (System.Exception e) { print("Cast failed."); }
		try { print((E)o); } catch (System.Exception e) { print("Cast failed."); }
		try { print((F)o); } catch (System.Exception e) { print("Cast failed."); }

		o = new CE();
		try { print((A)o); } catch (System.Exception e) { print("Cast failed."); }
		try { print((B)o); } catch (System.Exception e) { print("Cast failed."); }
		try { print((C)o); } catch (System.Exception e) { print("Cast failed."); }
		try { print((D)o); } catch (System.Exception e) { print("Cast failed."); }
		try { print((E)o); } catch (System.Exception e) { print("Cast failed."); }
		try { print((F)o); } catch (System.Exception e) { print("Cast failed."); }

		o = new CF();
		try { print((A)o); } catch (System.Exception e) { print("Cast failed."); }
		try { print((B)o); } catch (System.Exception e) { print("Cast failed."); }
		try { print((C)o); } catch (System.Exception e) { print("Cast failed."); }
		try { print((D)o); } catch (System.Exception e) { print("Cast failed."); }
		try { print((E)o); } catch (System.Exception e) { print("Cast failed."); }
		try { print((F)o); } catch (System.Exception e) { print("Cast failed."); }
	}
}
