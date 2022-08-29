using static SlimCS;
internal class Test
{
	delegate void Func();
	void DoIt(object oMissionData, bool bRetry, bool bSpendOnClick, bool bRaid, bool bSingleButton, bool forceHasEnoughCurrency, bool bClickableWhenDisabled, Func rSuccessCallback, string buttonSound = null)
	{
		print(oMissionData, bRetry, bSpendOnClick, bRaid, bSingleButton, forceHasEnoughCurrency, bClickableWhenDisabled, /*rSuccessCallback,*/ buttonSound);
	}

	void Nop()
	{
	}

	object m_oObject = 5;

	void Root()
	{
		DoIt(
			oMissionData: m_oObject,
			bRetry: true,
			bSpendOnClick: false,
			bRaid: false,
			bSingleButton: true,
			bClickableWhenDisabled: false,
			forceHasEnoughCurrency: true,
			rSuccessCallback: () =>
			{
				Nop();
			});
	}

	public static void Main ()
	{
		(new Test()).Root();
	}
}
