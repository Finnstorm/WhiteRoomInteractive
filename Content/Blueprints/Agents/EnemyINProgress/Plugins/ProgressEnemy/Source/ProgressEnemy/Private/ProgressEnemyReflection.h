#pragma once
#include "CoreMinimal.h"
#include "UObject/UnrealType.h"
#include "UObject/StructOnScope.h"

// The two shared components are Blueprint classes. Keep their integration in one place.
namespace ProgressEnemyReflection
{
    inline FProperty* Property(UObject* O, FName Name) { return O ? O->GetClass()->FindPropertyByName(Name) : nullptr; }
    inline double Number(UObject* O, FName Name, double Fallback = 0)
    {
        if (FNumericProperty* P = CastField<FNumericProperty>(Property(O, Name)))
        {
            const void* V = P->ContainerPtrToValuePtr<void>(O);
            return P->IsFloatingPoint() ? P->GetFloatingPointPropertyValue(V) : double(P->GetSignedIntPropertyValue(V));
        }
        return Fallback;
    }
    inline void SetNumber(UObject* O, FName Name, double Value)
    {
        if (FNumericProperty* P = CastField<FNumericProperty>(Property(O, Name)))
        {
            void* V = P->ContainerPtrToValuePtr<void>(O);
            if (P->IsFloatingPoint()) P->SetFloatingPointPropertyValue(V, Value);
            else P->SetIntPropertyValue(V, int64(Value));
        }
    }
    inline bool Bool(UObject* O, FName Name)
    {
        FBoolProperty* P = CastField<FBoolProperty>(Property(O, Name));
        return P && P->GetPropertyValue_InContainer(O);
    }
    inline void SetBool(UObject* O, FName Name, bool Value)
    {
        if (FBoolProperty* P = CastField<FBoolProperty>(Property(O, Name))) P->SetPropertyValue_InContainer(O, Value);
    }
    inline UObject* Object(UObject* O, FName Name)
    {
        FObjectPropertyBase* P = CastField<FObjectPropertyBase>(Property(O, Name));
        return P ? P->GetObjectPropertyValue_InContainer(O) : nullptr;
    }
    inline void SetObject(UObject* O, FName Name, UObject* Value)
    {
        if (FObjectPropertyBase* P = CastField<FObjectPropertyBase>(Property(O, Name)))
            if (!Value || Value->IsA(P->PropertyClass)) P->SetObjectPropertyValue_InContainer(O, Value);
    }
    inline bool Call(UObject* O, FName Name, TFunction<void(UFunction*, uint8*)> Setup = {})
    {
        UFunction* F = O ? O->FindFunction(Name) : nullptr;
        if (!F) return false;
        FStructOnScope Params(F);
        if (Setup) Setup(F, Params.GetStructMemory());
        O->ProcessEvent(F, Params.GetStructMemory());
        return true;
    }
    inline void ParamNumber(UFunction* F, uint8* Params, FName Name, double Value)
    {
        if (FNumericProperty* P = FindFProperty<FNumericProperty>(F, Name))
        {
            void* V = P->ContainerPtrToValuePtr<void>(Params);
            if (P->IsFloatingPoint()) P->SetFloatingPointPropertyValue(V, Value);
            else P->SetIntPropertyValue(V, int64(Value));
        }
    }
}
