using static SlimCS;
using System;

namespace Repro2
{
	sealed class Color
	{
		public int ID;

		public Color (int id)
		{
			print(id);
			this.ID = id;
		}

		public static Color Black = new Color (0);
		public static Color White = new Color (255);
		public static Color Transparent = new Color (-1);
	}

	static class ExtensionMethods
	{
		public static Color Transparent (this Color c)
		{
			print(c.ID);
			return Color.White;
		}
	}

	public static class Test
	{
		public static int Main()
		{
			var c = Color.Black.Transparent();
			print(c.ID);
			return 0;
		}
	}
}
