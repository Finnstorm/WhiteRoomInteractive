#include "Misc/AutomationTest.h"
#include "ProgressEnemyTypes.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProgressEnemyResourcesTest, "ProgressEnemy.Combat.Resources", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProgressEnemyResourcesTest::RunTest(const FString& Parameters)
{
    FProgressEnemyResources Model;
    TestTrue(TEXT("First taser shot"), Model.SpendTaser(6));
    TestEqual(TEXT("One shot left"), Model.TaserShots, 1);
    TestEqual(TEXT("No cooldown before second shot"), Model.TaserRecharge, 0.f);
    TestTrue(TEXT("Second taser shot"), Model.SpendTaser(6));
    TestFalse(TEXT("No third shot during recharge"), Model.SpendTaser(6));
    Model.Tick(5.9f, 3);
    TestFalse(TEXT("Cooldown is not complete early"), Model.SpendTaser(6));
    Model.Tick(.2f, 3);
    TestEqual(TEXT("Full two-shot magazine after cooldown"), Model.TaserShots, 2);
    TestEqual(TEXT("Unblocked damage"), Model.Absorb(20, false, 6), 20.f);
    TestEqual(TEXT("Unblocked hits consume no shield"), Model.BlockHits, 3);
    for (int32 I = 0; I < 3; ++I) TestEqual(TEXT("Shield absorbs 75 percent"), Model.Absorb(20, true, 6), 5.f);
    TestEqual(TEXT("Exhausted shield lets damage through"), Model.Absorb(20, true, 6), 20.f);
    Model.Tick(6, 5);
    TestEqual(TEXT("Recharge respects scaled shield capacity"), Model.BlockHits, 5);
    TestEqual(TEXT("No damage does not consume shield"), Model.Absorb(0, true, 6), 0.f);
    TestEqual(TEXT("Shield remains full"), Model.BlockHits, 5);
    return true;
}
#endif
