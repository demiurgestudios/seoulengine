using static SlimCS;
public static class Test
{
	public class X
	{
		public int A { get; set; }
		public int B { get; set; } = 1;
		int c; public int C { get { print("get_C"); return c; } set { print("set_C"); c = value; } }

		public void Print()
		{
			print(A); print(B); print(C);
		}
	}

	public static void Main()
	{
		var x = new X();

		x.Print();

		x.A = x.A; x.B = x.B; x.C = x.C; x.Print();

		x.A++; x.B++; x.C++; x.Print();
		++x.A; ++x.B; ++x.C; x.Print();
		x.A--; x.B--; x.C--; x.Print();
		--x.A; --x.B; --x.C; x.Print();

		x.A = x.A + x.A; x.B = x.B + x.B; x.C = x.C + x.C; x.Print();
		x.A += x.A; x.B += x.B; x.C += x.C; x.Print();

		x.A = x.A - x.A; x.B = x.B - x.B; x.C = x.C - x.C; x.Print();
		x.A -= x.A; x.B -= x.B; x.C -= x.C; x.Print();

		x.A = x.A * x.A; x.B = x.B * x.B; x.C = x.C * x.C; x.Print();
		x.A *= x.A; x.B *= x.B; x.C *= x.C; x.Print();

		x.A = x.A / (x.A+1); x.B = x.B / (x.B+1); x.C = x.C / (x.C+1); x.Print();
		x.A /= (x.A+1); x.B /= (x.B+1); x.C /= (x.C+1); x.Print();

		x.A = x.A % (x.A+1); x.B = x.B % (x.B+1); x.C = x.C % (x.C+1); x.Print();
		x.A %= (x.A+1); x.B %= (x.B+1); x.C %= (x.C+1); x.Print();

		x.A = -x.A; x.B = -x.B; x.C = -x.C; x.Print();
		x.A = +x.A; x.B = +x.B; x.C = +x.C; x.Print();
		x.A = ~x.A; x.B = ~x.B; x.C = ~x.C; x.Print();

		x.A = x.A << x.A; x.B = x.B << x.B; x.C = x.C << x.C; x.Print();
		x.A <<= x.A; x.B <<= x.B; x.C <<= x.C; x.Print();

		x.A = x.A >> x.A; x.B = x.B >> x.B; x.C = x.C >> x.C; x.Print();
		x.A >>= x.A; x.B >>= x.B; x.C >>= x.C; x.Print();

		x.A = x.A & x.A; x.B = x.B & x.B; x.C = x.C & x.C; x.Print();
		x.A &= x.A; x.B &= x.B; x.C &= x.C; x.Print();

		x.A = x.A ^ x.A; x.B = x.B ^ x.B; x.C = x.C ^ x.C; x.Print();
		x.A ^= x.A; x.B ^= x.B; x.C ^= x.C; x.Print();

		x.A = x.A | x.A; x.B = x.B | x.B; x.C = x.C | x.C; x.Print();
		x.A |= x.A; x.B |= x.B; x.C |= x.C; x.Print();
	}
}
