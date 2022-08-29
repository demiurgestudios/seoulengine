namespace System
{
	public static class Activator
	{
		/// <summary>
		/// Creates an instance of the specified type using that type's default constructor.
		/// </summary>
		/// <param name="type">The type of object to create.</param>
		/// <returns>A reference to the newly created object.</returns>
		public static extern object CreateInstance(Type type);
	}
}
