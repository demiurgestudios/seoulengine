class list <A> {
	public class Cons <T> : list <T> {   }
	public class Nil <T> : list <T> {  }
}

class C {
	public static void Rev<T> (list <T> y) {
		if (y is list<object>.Cons<T>)
			SlimCS.print ("Cons");
		if (y is list<object>.Nil<T>)
			SlimCS.print ("Nil");
	}
}

class M {
	public static void Main () { 
		C.Rev (new list<object>.Cons <string> ());
		C.Rev (new list<object>.Nil <string> ());
	 }
}
