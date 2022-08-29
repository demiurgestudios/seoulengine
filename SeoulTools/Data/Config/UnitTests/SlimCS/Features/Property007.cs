using static SlimCS;

public class A
{
	int m_Value;
	public int Value
	{
		set
		{
			m_Value = value;
			OnUpdate();
		}

		get
		{
			return m_Value;
		}
	}

	void OnUpdate()
	{
	}
}

public static class Test
{
	public static void Main()
	{
		var a = new A();
		a.Value = 5;
		print(a.Value);
	}
}
