using System;

namespace MonoVirtuals
{
	class X { }
	class Y : X { }

	class A
	{
		public virtual int f (X o)
		{
			SlimCS.print ("In A for X");
			return 5;
		}

		public virtual int f (Y o)
		{
			SlimCS.print ("In A for Y");
			return 10;
		}

		public virtual int this[X o]
		{
			get
			{
				SlimCS.print ("In A for X");
				return 5;
			}
		}

		public virtual int this[Y o]
		{
			get
			{
				SlimCS.print ("In A for Y");
				return 10;
			}
		}
	}

	class B : A
	{
		public override int f (X o)
		{
			base.f (o);
			throw new ApplicationException ("should not be called");
		}

		public override int this[X o]
		{
			get
			{
				base.f (o);
				throw new ApplicationException ("should not be called");
			}
		}
	}

	class C : B
	{
		public override int f (X o)
		{
			SlimCS.print ("In C for X");
			return base.f (o);
		}

		public override int f (Y o)
		{
			SlimCS.print ("In C for Y");
			return base.f (o);
		}

		public override int this[X o]
		{
			get
			{
				SlimCS.print ("In C for X");
				return base.f (o);
			}
		}

		public override int this[Y o]
		{
			get
			{
				SlimCS.print ("In C for Y");
				return base.f (o);
			}
		}
	}

	class MainClass
	{
		public static int Main ()
		{
			var o = new Y ();
			var c = new C ();
			if (c.f (o) != 10)
				return 1;

			if (c[o] != 10)
				return 2;

			return 0;
		}
	}
}
