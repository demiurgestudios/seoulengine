using System;

class Program
{
    public static int Main ()
    {
        SlimCS.print (M (1));
        try {
            SlimCS.print (M (null));
        } catch (Exception) {
            SlimCS.print ("thrown");
            return 0;
        }

        return 1;
    }

    static string M (object data)
    {
        return data?.ToString () ?? throw null;
    }
}