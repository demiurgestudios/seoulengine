using static SlimCS;
public static class Test
{
	public static int GetHash(int key)
	{
		int hash = key;
		hash = (int)((hash + unchecked((int)0x7ed55d16)) + (hash << 12));
		hash = (int)((hash ^ unchecked((int)0xc761c23c)) ^ (hash >> 19));
		hash = (int)((hash + unchecked((int)0x165667b1)) + (hash << 5));
		hash = (int)((hash + unchecked((int)0xd3a2646c)) ^ (hash << 9));
		hash = (int)((hash + unchecked((int)0xfd7046c5)) + (hash << 3));
		hash = (int)((hash ^ unchecked((int)0xb55a4f09)) ^ (hash >> 16));
		return hash;
	}

	public static void Main()
	{
		for (int i = 0; i < 100000; ++i)
		{
			print(GetHash(i));
		}
	}
}
