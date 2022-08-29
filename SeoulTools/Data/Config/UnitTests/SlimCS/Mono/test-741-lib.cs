// Compiler options: -target:library

public interface IA
{
	string Prop { get; }
}

public interface IAA
{
	double NestedProp { set; }
}

public interface IB : IAA
{
	bool Prop { get; }
	int NestedProp { set; }
}

public interface IFoo : IA, IB
{
	int Prop { get; }
}
