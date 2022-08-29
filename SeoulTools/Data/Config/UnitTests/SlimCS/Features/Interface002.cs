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
		print(o as A); print(o is A);
		print(o as B); print(o is B);
		print(o as C); print(o is C);
		print(o as D); print(o is D);
		print(o as E); print(o is E);
		print(o as F); print(o is F);

		o = new CB();
		print(o as A); print(o is A);
		print(o as B); print(o is B);
		print(o as C); print(o is C);
		print(o as D); print(o is D);
		print(o as E); print(o is E);
		print(o as F); print(o is F);

		o = new CC();
		print(o as A); print(o is A);
		print(o as B); print(o is B);
		print(o as C); print(o is C);
		print(o as D); print(o is D);
		print(o as E); print(o is E);
		print(o as F); print(o is F);

		o = new CD();
		print(o as A); print(o is A);
		print(o as B); print(o is B);
		print(o as C); print(o is C);
		print(o as D); print(o is D);
		print(o as E); print(o is E);
		print(o as F); print(o is F);

		o = new CE();
		print(o as A); print(o is A);
		print(o as B); print(o is B);
		print(o as C); print(o is C);
		print(o as D); print(o is D);
		print(o as E); print(o is E);
		print(o as F); print(o is F);

		o = new CF();
		print(o as A); print(o is A);
		print(o as B); print(o is B);
		print(o as C); print(o is C);
		print(o as D); print(o is D);
		print(o as E); print(o is E);
		print(o as F); print(o is F);
	}
}
