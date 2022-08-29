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

		public void Test()
		{
			Print();

			A = A; B = B; C = C; Print();

			A++; B++; C++; Print();
			++A; ++B; ++C; Print();
			A--; B--; C--; Print();
			--A; --B; --C; Print();

			A = A + A; B = B + B; C = C + C; Print();
			A += A; B += B; C += C; Print();

			A = A - A; B = B - B; C = C - C; Print();
			A -= A; B -= B; C -= C; Print();

			A = A * A; B = B * B; C = C * C; Print();
			A *= A; B *= B; C *= C; Print();

			A = A / (A+1); B = B / (B+1); C = C / (C+1); Print();
			A /= (A+1); B /= (B+1); C /= (C+1); Print();

			A = A % (A+1); B = B % (B+1); C = C % (C+1); Print();
			A %= (A+1); B %= (B+1); C %= (C+1); Print();

			A = -A; B = -B; C = -C; Print();
			A = +A; B = +B; C = +C; Print();
			A = ~A; B = ~B; C = ~C; Print();

			A = A << A; B = B << B; C = C << C; Print();
			A <<= A; B <<= B; C <<= C; Print();

			A = A >> A; B = B >> B; C = C >> C; Print();
			A >>= A; B >>= B; C >>= C; Print();

			A = A & A; B = B & B; C = C & C; Print();
			A &= A; B &= B; C &= C; Print();

			A = A ^ A; B = B ^ B; C = C ^ C; Print();
			A ^= A; B ^= B; C ^= C; Print();

			A = A | A; B = B | B; C = C | C; Print();
			A |= A; B |= B; C |= C; Print();
		}
	}

	public static void Main()
	{
		var x = new X();
		x.Test();
	}
}
