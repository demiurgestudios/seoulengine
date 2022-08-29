using c = SlimCS;
using SlimCS2 = SlimCS;

namespace A {
	namespace B {
		class C {
			public static void Hola () {
				c.print ("Hola!");
			}
		}
	}
}

namespace X {
	namespace Y {
		namespace Z {
			class W {
				public static void Ahoj () {
					SlimCS.print ("Ahoj!");
				}
			}
		}
	}
}

namespace Foo {

  class SlimCS {
	static void X() {
	  SlimCS2.print("FOO");
	}
  }
}

class App {
	public static int Main () {
		A.B.C.Hola ();
		X.Y.Z.W.Ahoj ();

		return 0;
	}
}
