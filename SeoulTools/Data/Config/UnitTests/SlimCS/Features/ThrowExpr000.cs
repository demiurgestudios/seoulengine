using static SlimCS;
using System;

class Test
{
	public static void Main ()
	{
		Func<object> f = () => throw null;
		try { f(); } catch (Exception e) { print(e.Message); }
		try { (new Test()).Test1(); } catch (Exception e) { print(e.Message); }
		try { (new Test()).Test2(); } catch (Exception e) { print(e.Message); }
		try { Test3(0); } catch (Exception e) { print(e.Message); }
		try { Test3(1); } catch (Exception e) { print(e.Message); }
		try { print((new Test())[0]); } catch (Exception e) { print(e.Message); }
		try { (new Test()).Event += () => {}; } catch (Exception e) { print(e.Message); }
		try { (new Test()).Event -= () => {}; } catch (Exception e) { print(e.Message); }
		try { (new Test()).TestExpr_1(false); } catch (Exception e) { print(e.Message); }
		try { (new Test()).TestExpr_1(true); } catch (Exception e) { print(e.Message); }
		try { (new Test()).TestExpr_2(false); } catch (Exception e) { print(e.Message); }
		try { (new Test()).TestExpr_2(true); } catch (Exception e) { print(e.Message); }
		try { (new Test()).TestExpr_3(null); } catch (Exception e) { print(e.Message); }
		try { (new Test()).TestExpr_3("Hello World"); } catch (Exception e) { print(e.Message); }
		try { (new Test()).TestExpr_4(); } catch (Exception e) { print(e.Message); }
		try { (new Test()).TestExpr_5(); } catch (Exception e) { print(e.Message); }
	}

	public int Test1 () => throw null;

	object Foo ()
	{
		return null;
	}

	public object Test2 () => Foo () ?? throw null;

	static void Test3 (int z) => throw null;

	int this [int x] {
		get => throw null;
	}

	public event Action Event {
		add => throw null;
		remove => throw null;
	}

	void TestExpr_1 (bool b)
	{
		int x = b ? throw new NullReferenceException () : 1;
		print(x);
	}

	void TestExpr_2 (bool b)
	{
		int x = b ? 2 : throw new NullReferenceException ();
		print(x);
	}

	void TestExpr_3 (string s)
	{
		s = s ?? throw new NullReferenceException ();
		print(s);
	}

	void TestExpr_4 ()
	{
		throw new Exception () ?? throw new NullReferenceException() ?? throw null;
	}

	void TestExpr_5 ()
	{
		Action a = () => throw new Exception () ?? throw new NullReferenceException() ?? throw null;
	}
}
