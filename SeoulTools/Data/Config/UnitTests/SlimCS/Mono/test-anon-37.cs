using System;

public class DelegateInit {
    public delegate void FooDelegate();

    public static readonly FooDelegate _print =
        delegate() {
            SlimCS.print("delegate!");
        };

    public static void Main(string[] args) {
        _print();
    }
}