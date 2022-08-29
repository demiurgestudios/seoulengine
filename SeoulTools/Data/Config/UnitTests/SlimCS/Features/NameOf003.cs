using static SlimCS;
using System;

public class Person
{
	public Car MyCar { get; set; }
}

public class Car
{
	public int Year { get; set; }
}

enum EE
{
	K
}

public class Test
{
	class Nested
	{
	}

	public static Person MyPerson1 { get; } = new Person();
	public static Person MyPerson2 = new Person();
	public const Person MyPerson3 = null;
	public static event Action Act = null;

	void ParameterTest (Person ParPerson)
	{
		print(nameof (ParPerson.MyCar.Year));
	}

	public static int Main ()
	{
		string name;

		name = nameof (MyPerson1.MyCar.Year); print(name);
		if (name != "Year")
			return 1;

		name = nameof (MyPerson2.MyCar.Year); print(name);
		if (name != "Year")
			return 2;

		name = nameof (MyPerson3.MyCar.Year); print(name);
		if (name != "Year")
			return 3;

		name = nameof (Act.Method.MemberType); print(name);
		if (name != "MemberType")
			return 4;

		name = nameof (EE.K.ToString); print(name);
		if (name != "ToString")
			return 5;

		name = nameof (int.ToString); print(name);
		if (name != "ToString")
			return 6;

		Person LocPerson = null;
		name = nameof (LocPerson.MyCar.Year); print(name);
		if (name != "Year")
			return 7;

		return 0;
	}
}
