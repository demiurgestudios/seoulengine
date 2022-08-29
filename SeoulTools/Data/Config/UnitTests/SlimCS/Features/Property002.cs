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

			this.A = A; this.B = B; this.C = C; Print();

			this.A++; this.B++; this.C++; Print();
			++A; ++B; ++C; Print();
			this.A--; this.B--; this.C--; Print();
			--A; --B; --C; Print();

			this.A = this.A + A; this.B = B + this.B; this.C = this.C + this.C; Print();
			this.A += A; B += this.B; this.C += this.C; Print();

			this.A = A - this.A; this.B = this.B - B; this.C = this.C - this.C; Print();
			this.A -= A; B -= this.B; this.C -= this.C; Print();

			this.A = this.A * A; this.B = B * this.B; this.C = this.C * this.C; Print();
			this.A *= A; B *= this.B; this.C *= this.C; Print();

			this.A = this.A / (A+1); this.B = B / (this.B+1); this.C = this.C / (this.C+1); Print();
			this.A /= (A+1); B /= (this.B+1); this.C /= (this.C+1); Print();

			this.A = this.A % (A+1); this.B = B % (this.B+1); this.C = this.C % (this.C+1); Print();
			this.A %= (A+1); B %= (this.B+1); this.C %= (this.C+1); Print();

			this.A = -A; B = -this.B; this.C = -this.C; Print();
			this.A = +A; B = +this.B; this.C = +this.C; Print();
			this.A = ~A; B = ~this.B; this.C = ~this.C; Print();

			this.A = this.A << A; this.B = B << this.B; this.C = this.C << this.C; Print();
			this.A <<= A; B <<= this.B; this.C <<= this.C; Print();

			this.A = this.A >> A; this.B = B >> this.B; this.C = this.C >> this.C; Print();
			this.A >>= A; B >>= this.B; this.C >>= this.C; Print();

			this.A = this.A & A; this.B = B & this.B; this.C = this.C & this.C; Print();
			this.A &= A; B &= this.B; this.C &= this.C; Print();

			this.A = this.A ^ A; this.B = B ^ this.B; this.C = this.C ^ this.C; Print();
			this.A ^= A; B ^= this.B; this.C ^= this.C; Print();

			this.A = this.A | A; this.B = B | this.B; this.C = this.C | this.C; Print();
			this.A |= A; B |= this.B; this.C |= this.C; Print();
		}
	}

	public static void Main()
	{
		var x = new X();
		x.Test();
	}
}
