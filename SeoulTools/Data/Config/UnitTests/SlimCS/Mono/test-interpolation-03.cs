using System;

public static class Test {
	public static void Main() {
		RunTest(() => SlimCS.print ($"Initializing the map took {1}ms"));
	}

	static void RunTest (Action callback)
	 {
		callback();
	}
}