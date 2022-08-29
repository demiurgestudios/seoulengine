using static SlimCS;
using System;

class Test
{
	public static void Main ()
	{
		object o;
		o = false;
		try { print((bool)o); } catch (System.Exception e) { print(e.Message); }
		try { print((double)o); } catch (System.Exception e) { print("Cast failed."); }
		// TODO: try { print((int)o); } catch (System.Exception e) { print(e.Message); }
		try { print((string)o); } catch (System.Exception e) { print("Cast failed."); }

		o = 1.0;
		try { print((bool)o); } catch (System.Exception e) { print("Cast failed."); }
		try { print((double)o); } catch (System.Exception e) { print(e.Message); }
		// TODO: try { print((int)o); } catch (System.Exception e) { print(e.Message); }
		try { print((string)o); } catch (System.Exception e) { print("Cast failed."); }

		o = "Hello World";
		try { print((bool)o); } catch (System.Exception e) { print("Cast failed."); }
		try { print((double)o); } catch (System.Exception e) { print("Cast failed."); }
		// TODO: try { print((int)o); } catch (System.Exception e) { print(e.Message); }
		try { print((string)o); } catch (System.Exception e) { print(e.Message); }
	}
}
