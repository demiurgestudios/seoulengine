using static SlimCS;
using System;

class A {}
class B {}
class C {}

class Test
{
	static void TestBool()
	{
		var a = new GenericList<bool>();
		print(a);
		print(a.Count);

		a.Add(false);
		print(a[0]);
		a.Add(true);
		print(a[0], a[1]);
		a.Add(false);
		print(a[0], a[1], a[2]);
	}

	static void TestInt()
	{
		var a = new GenericList<int>();
		print(a);
		print(a.Count);

		a.Add(1);
		print(a[0]);
		a.Add(2);
		print(a[0], a[1]);
		a.Add(3);
		print(a[0], a[1], a[2]);
		a.Add();
		print(a[0], a[1], a[2], a[3]);
	}

	static void TestObject()
	{
		var a = new GenericList<object>();
		print(a);
		print(a.Count);

		a.Add(new A());
		print(a[0]);
		a.Add(new B());
		print(a[0], a[1]);
		a.Add(null);
		print(a[0], a[1], a[2]);
		a.Add(new C());
		print(a[0], a[1], a[2], a[3]);
	}

	static void TestString()
	{
		var a = new GenericList<string>();
		print(a);
		print(a.Count);

		a.Add("A");
		print(a[0]);
		a.Add("B");
		print(a[0], a[1]);
		a.Add("C");
		print(a[0], a[1], a[2]);
		a.Add();
		print(a[0], a[1], a[2], a[3]);
	}

	public static void Main ()
	{
		TestBool();
		TestInt();
		TestObject();
		TestString();
	}
}

static class ArrayUtil
{
	public static U[] Resize<U>(U[] a, int count)
	{
		var existing = (null == a ? 0 : a.Length);
		if (count == existing)
		{
			return a;
		}

		var ret = new U[count];
		var copy = (existing < count ? existing : count);
		for (int i = 0; i < copy; ++i)
		{
			ret[i] = a[i];
		}

		return ret;
	}
}

class GenericList<T>
{
	int m_Count;
	T[] m_a;

	public void Add()
	{
		Add(default(T));
	}

	public void Add(T input)
	{
		m_a = ArrayUtil.Resize(m_a, m_Count + 1);
		m_a[m_Count] = input;
		++m_Count;
	}

	public int Count { get { return m_Count; } }

	public T this[int i]
	{
		get
		{
			return m_a[i];
		}

		set
		{
			m_a[i] = value;
		}
	}
}
