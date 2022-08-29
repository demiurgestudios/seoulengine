using System;
using System.Linq.Expressions;

namespace TestGenericsSubtypeMatching
{
	public class Sender<T> : IDisposable
	{

		public void DoSend<TMessage> (Action<T> action)
		{
			using (Sender<T> sub = new Sender<T> ())
			{
				Send (t =>
				{
					action(t);
					sub.ActionOnObject (t);
				});
			}
		}
		
		private static void Send (Action<T> action)
		{
		}
		
		void ActionOnObject (object o)
		{
			o.ToString ();
		}
	
		#region IDisposable implementation
		public void Dispose ()
		{
			SlimCS.print ("Dispose!");
		}
		
		#endregion
	}
	
	class C
	{
		public static int Main ()
		{
			new Sender<int> ().DoSend<bool> (l => SlimCS.print (l));
			return 0;
		}
	}
}
