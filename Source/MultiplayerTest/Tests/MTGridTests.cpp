#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Inventory/MTInventoryGrid.h"
#include "Inventory/MTInventoryComponent.h"
#include "Items/MTItemData.h"
#include "Online/MTSessionFilter.h"

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FMTGridTests, "MultiplayerTest.Inventory.Grid",
								  EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

void FMTGridTests::GetTests(TArray<FString>& Names, TArray<FString>& Commands) const
{
	for (const TCHAR* Name : {TEXT("EmptyAccepts2x1"), TEXT("RightBoundaryRejects"), TEXT("RightBoundaryAccepts"),
							  TEXT("NegativeCoordinates"), TEXT("BottomOverflowRejected"), TEXT("OverlapRejected"),
							  TEXT("MoveFreesOldCells"), TEXT("MoveOutsideRejected"),
							  TEXT("FailedMovePreservesPosition"), TEXT("FindFreePosition"), TEXT("FullRejects"),
							  TEXT("RemoveExisting"), TEXT("InvalidHandleRejected"), TEXT("SelfOverlapAllowed"),
							  TEXT("InvalidFootprintRejected"), TEXT("StableHandles"), TEXT("NoAuthorityRejected")})
	{
		Names.Add(Name);
		Commands.Add(Name);
	}
}

bool FMTGridTests::RunTest(const FString& Case)
{
	using namespace MTInventoryGrid;
	TArray<FMTInventoryEntry> Items;
	UMTItemData* Medkit = NewObject<UMTItemData>();
	Medkit->GridSize = FIntPoint(2, 1);
	if (Case == TEXT("EmptyAccepts2x1"))
	{
		TestTrue(TEXT("2x1 at (0,0)"), IsAreaFree(Items, FIntPoint(0, 0), Medkit->GridSize));
	}
	else if (Case == TEXT("RightBoundaryRejects"))
	{
		TestFalse(TEXT("2x1 at (7,0)"), IsAreaFree(Items, FIntPoint(7, 0), Medkit->GridSize));
	}
	else if (Case == TEXT("RightBoundaryAccepts"))
	{
		TestTrue(TEXT("2x1 at (6,0)"), IsAreaFree(Items, FIntPoint(6, 0), Medkit->GridSize));
	}
	else if (Case == TEXT("NegativeCoordinates"))
	{
		TestFalse(TEXT("Negative X"), IsAreaFree(Items, FIntPoint(-1, 0), Medkit->GridSize));
		TestFalse(TEXT("Negative Y"), IsAreaFree(Items, FIntPoint(0, -1), Medkit->GridSize));
	}
	else if (Case == TEXT("BottomOverflowRejected"))
	{
		Medkit->GridSize = FIntPoint(1, 2);
		TestFalse(TEXT("1x2 at bottom row"), IsAreaFree(Items, FIntPoint(0, 7), Medkit->GridSize));
	}
	else if (Case == TEXT("InvalidFootprintRejected"))
	{
		for (FIntPoint Size :
			 {FIntPoint(0, 1), FIntPoint(-1, 1), FIntPoint(9, 1), FIntPoint(1, 9), FIntPoint(MAX_int32, 1)})
		{
			Medkit->GridSize = Size;
			TestFalse(TEXT("Invalid item size"), TryAddItem(Items, Medkit));
		}
		TestFalse(TEXT("Null item"), TryAddItem(Items, nullptr));
	}
	else if (Case == TEXT("NoAuthorityRejected"))
	{
		UMTInventoryComponent* Inventory = NewObject<UMTInventoryComponent>();
		TestFalse(TEXT("No owning authority cannot add"), Inventory->TryAddItem(Medkit));
		TestFalse(TEXT("No owning authority cannot move"), Inventory->TryMoveItem(FGuid::NewGuid(), FIntPoint(0, 0)));
		TestFalse(TEXT("No owning authority cannot remove"), Inventory->RemoveItem(FGuid::NewGuid()));
	}
	else
	{
		if (!TestTrue(TEXT("Initial item added"), TryAddItem(Items, Medkit)))
		{
			return false;
		}
		const FGuid Handle = Items[0].Handle;
		if (Case == TEXT("OverlapRejected"))
		{
			TestFalse(TEXT("Occupied cell"), IsAreaFree(Items, FIntPoint(1, 0), Medkit->GridSize));
		}
		else if (Case == TEXT("MoveFreesOldCells"))
		{
			TestTrue(TEXT("Move"), TryMoveItem(Items, Handle, FIntPoint(3, 4)));
			TestTrue(TEXT("Old cells free"), IsAreaFree(Items, FIntPoint(0, 0), Medkit->GridSize));
			TestFalse(TEXT("New cells occupied"), IsAreaFree(Items, FIntPoint(3, 4), Medkit->GridSize));
		}
		else if (Case == TEXT("MoveOutsideRejected"))
		{
			for (FIntPoint P : {FIntPoint(-1, 0), FIntPoint(7, 0), FIntPoint(0, 8), FIntPoint(MAX_int32, 0)})
			{
				TestFalse(TEXT("Outside"), TryMoveItem(Items, Handle, P));
			}
			TestEqual(TEXT("Unchanged"), Items[0].Position, FIntPoint(0, 0));
		}
		else if (Case == TEXT("FailedMovePreservesPosition"))
		{
			TestFalse(TEXT("Move beyond right edge"), TryMoveItem(Items, Handle, FIntPoint(7, 0)));
			TestEqual(TEXT("Original position preserved"), Items[0].Position, FIntPoint(0, 0));
		}
		else if (Case == TEXT("FindFreePosition"))
		{
			FIntPoint P;
			TestTrue(TEXT("Found"), FindFirstAvailablePosition(Items, Medkit->GridSize, P));
			TestEqual(TEXT("Row-major first fit"), P, FIntPoint(2, 0));
		}
		else if (Case == TEXT("FullRejects"))
		{
			for (int32 I = 1; I < 32; ++I)
			{
				TestTrue(TEXT("Fill 64 cells"), TryAddItem(Items, Medkit));
			}
			TestFalse(TEXT("Full"), TryAddItem(Items, Medkit));
			TestEqual(TEXT("No extra entry"), Items.Num(), 32);
		}
		else if (Case == TEXT("RemoveExisting"))
		{
			TestTrue(TEXT("Remove"), RemoveItem(Items, Handle));
			TestTrue(TEXT("Empty"), Items.IsEmpty());
			TestFalse(TEXT("No duplicate removal"), RemoveItem(Items, Handle));
		}
		else if (Case == TEXT("InvalidHandleRejected"))
		{
			TestFalse(TEXT("Zero handle"), TryMoveItem(Items, FGuid(), FIntPoint(2, 0)));
			TestFalse(TEXT("Unknown handle"), TryMoveItem(Items, FGuid::NewGuid(), FIntPoint(2, 0)));
			TestFalse(TEXT("Remove unknown"), RemoveItem(Items, FGuid::NewGuid()));
			TestEqual(TEXT("Unchanged count"), Items.Num(), 1);
		}
		else if (Case == TEXT("SelfOverlapAllowed"))
		{
			TestTrue(TEXT("Overlap with self"), TryMoveItem(Items, Handle, FIntPoint(1, 0)));
			TestTrue(TEXT("Same position"), TryMoveItem(Items, Handle, FIntPoint(1, 0)));
		}
		else if (Case == TEXT("StableHandles"))
		{
			TestTrue(TEXT("Second copy"), TryAddItem(Items, Medkit));
			const FGuid Other = Items[1].Handle;
			TestNotEqual(TEXT("Unique handles for same data"), Handle, Other);
			TestTrue(TEXT("Remove first"), RemoveItem(Items, Handle));
			TestTrue(TEXT("Move survivor after index shift"), TryMoveItem(Items, Other, FIntPoint(6, 7)));
			TestEqual(TEXT("Stable ID"), Items[0].Handle, Other);
		}
		else
		{
			AddError(TEXT("Unknown case"));
			return false;
		}
	}
	return true;
}

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FMTLobbyFilterTests, "MultiplayerTest.Sessions.Filter",
								  EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

void FMTLobbyFilterTests::GetTests(TArray<FString>& Names, TArray<FString>& Commands) const
{
	for (const TCHAR* Name : {TEXT("ExactTag"), TEXT("WrongTag"), TEXT("MissingTag"), TEXT("PrivateExcluded"),
							  TEXT("CaseSensitive"), TEXT("NotAdvertised"), TEXT("MissingPublicFlag")})
	{
		Names.Add(Name);
		Commands.Add(Name);
	}
}

bool FMTLobbyFilterTests::RunTest(const FString& Case)
{
	FOnlineSessionSettings S;
	S.bShouldAdvertise = true;
	S.Set(MTSessionFilter::TagKey, MTSessionFilter::TagValue, EOnlineDataAdvertisementType::ViaOnlineService);
	S.Set(MTSessionFilter::PublicKey, 1, EOnlineDataAdvertisementType::ViaOnlineService);
	if (Case == TEXT("WrongTag"))
	{
		S.Set(MTSessionFilter::TagKey, FString(TEXT("OtherProject")));
	}
	else if (Case == TEXT("MissingTag"))
	{
		S.Settings.Remove(MTSessionFilter::TagKey);
	}
	else if (Case == TEXT("PrivateExcluded"))
	{
		S.bShouldAdvertise = false;
		S.Set(MTSessionFilter::PublicKey, 0);
	}
	else if (Case == TEXT("CaseSensitive"))
	{
		S.Set(MTSessionFilter::TagKey, FString(TEXT("ue5.8_mpsteamtest")));
	}
	else if (Case == TEXT("NotAdvertised"))
	{
		S.bShouldAdvertise = false;
	}
	else if (Case == TEXT("MissingPublicFlag"))
	{
		S.Settings.Remove(MTSessionFilter::PublicKey);
	}
	TestEqual(TEXT("Public browser acceptance"), MTSessionFilter::IsPublicLobby(S), Case == TEXT("ExactTag"));
	if (Case == TEXT("PrivateExcluded"))
	{
		TestTrue(TEXT("Private project invite remains eligible"), MTSessionFilter::IsProjectLobby(S));
	}
	return true;
}
#endif
