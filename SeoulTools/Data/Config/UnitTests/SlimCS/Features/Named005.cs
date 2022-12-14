using static SlimCS;
class Test
{
	static int Foo (string packageId, int version)
	{
		print(packageId, version);
		return Foo (packageId, version, ignoreDependencies: false, allowPrereleaseVersions: false);
	}

	static int Foo (string packageId, int version, bool ignoreDependencies, bool allowPrereleaseVersions)
	{
		print(packageId, version, ignoreDependencies, allowPrereleaseVersions);
		return 1;
	}

	static int Foo (double package, bool ignoreDependencies, bool allowPrereleaseVersions, bool ignoreWalkInfo)
	{
		print(package, ignoreDependencies, allowPrereleaseVersions, ignoreWalkInfo);
		return 2;
	}

	public static int Main ()
	{
		if (Foo ("", 1) != 1)
			return 1;

		return 0;
	}
}
