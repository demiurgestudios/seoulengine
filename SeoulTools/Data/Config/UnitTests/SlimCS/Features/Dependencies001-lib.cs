using static SlimCS;
namespace C.D.E
{
	class F : B.B
	{
		int m_i = 4789;

		protected new int MI { get { return m_i; } }

		public static new int Method() { return 35; }
		public override int Method(B.B b) { return m_i; }
	}
}

namespace B
{
	class B : A.A
	{
		int m_i = 6;
		static int s_iProperty = 259;

		protected new int MI { get { return m_i; } }

		public int I { get { return base.MI; } }
		public static new int Method() { return 8; }

		public static new int Property { get { return s_iProperty; } set { s_iProperty = value; } }
	}
}
