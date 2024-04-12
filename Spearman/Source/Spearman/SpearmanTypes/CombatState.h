#pragma once

UENUM(BlueprintType)
enum class ECombatState : uint8
{
	ECS_Idle UMETA(DisplayName = "Idle"),
	ECS_Attacking UMETA(DisplayName = "Attacking"),
	ECS_Stunned UMETA(DisplayName = "Stunned"),

	ECS_SuperArmor UMETA(DisplayName = "SuperArmor"),

	ECS_MAX UMETA(DisplayName = "DefaultMAX")
};