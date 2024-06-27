#pragma once
#include "Fyrion/Core/Registry.hpp"
#include "Fyrion/Core/UUID.hpp"

namespace Fyrion
{
    class Asset;
    class AssetDirectory;
    struct SubobjectApi;


    struct SubobjectApi
    {
        void (*  SetPrototype)(VoidPtr subobject, VoidPtr prototype);
        void (*  SetOwner)(VoidPtr subobject, VoidPtr owner);
        usize (* GetOwnedObjectsCount)(VoidPtr subobject);
        void (*  GetOwnedObjects)(VoidPtr subobject, Span<VoidPtr> assets);
        void (*  Remove)(VoidPtr subobject, VoidPtr object);
        TypeID (*GetTypeId)();
    };

    class SubobjectBase
    {
    public:
        virtual              ~SubobjectBase() = default;
        virtual SubobjectApi GetApi() = 0;
        virtual Asset*       GetOwner() = 0;
    };

    template <typename Type>
    class Subobject : public SubobjectBase
    {
    public:
        void Add(Type* object)
        {
            FY_ASSERT(object, "asset is null");
            if constexpr (Traits::IsBaseOf<Asset, Type>)
            {
                FY_ASSERT(!object->subobjectOf, "asset is already a subobject");
                object->subobjectOf = this;
            }
            objects.EmplaceBack(object);
        }

        void Remove(Type* object)
        {
            FY_ASSERT(object, "asset is null");
            if (Type** it = FindFirst(objects.begin(), objects.end(), object))
            {
                objects.Erase(it);
                if constexpr (Traits::IsBaseOf<Asset, Type>)
                {
                    object->subobjectOf = nullptr;
                }
            }
        }

        usize Count() const
        {
            usize total = objects.Size();
            if (prototype != nullptr)
            {
                total += prototype->Count();
            }
            return total;
        }

        void Get(Span<Type*> p_objects) const
        {
            GetTo(p_objects, 0);
        }

        //TODO make a proper iterator to avoid a heap allocation in Array
        Array<Type*> GetAsArray() const
        {
            Array<Type*> ret(Count());
            Get(ret);
            return ret;
        }

        Span<Type*> GetOwnedObjects() const
        {
            return objects;
        }

        SubobjectApi GetApi() override;

        Asset* GetOwner() override
        {
            return owner;
        }

        friend class TypeApiInfo<Subobject>;

    private:
        Subobject*   prototype = {};
        Asset*       owner{};
        Array<Type*> objects;

        void GetTo(Span<Type*> p_objects, usize pos) const
        {
            if (prototype != nullptr)
            {
                prototype->GetTo(p_objects, pos);
            }

            for (Type* object : objects)
            {
                p_objects[pos++] = object;
            }
        }
    };

    template <typename Type>
    struct TypeApiInfo<Subobject<Type>>
    {
        static void SetPrototypeImpl(VoidPtr subobject, VoidPtr prototype)
        {
            static_cast<Subobject<Type>*>(subobject)->prototype = static_cast<Subobject<Type>*>(prototype);
        }

        static usize GetOwnedObjectsCount(VoidPtr subobject)
        {
            return static_cast<Subobject<Type>*>(subobject)->objects.Size();
        }

        static void GetOwnedObjects(VoidPtr ptr, Span<VoidPtr> retAssets)
        {
            auto subobject = static_cast<Subobject<Type>*>(ptr);

            usize pos = 0;
            for (Type* asset : subobject->objects)
            {
                retAssets[pos++] = asset;
            }
        }

        static void RemoveImpl(VoidPtr subobject, VoidPtr object)
        {
            static_cast<Subobject<Type>*>(subobject)->Remove(static_cast<Type*>(object));
        }

        static void SetOwnerImpl(VoidPtr subobject, VoidPtr owner)
        {
            static_cast<Subobject<Type>*>(subobject)->owner = static_cast<Asset*>(owner);
        }

        static TypeID GetTypeIdImpl()
        {
            return GetTypeID<Type>();
        }

        static void ExtractApi(VoidPtr pointer)
        {
            SubobjectApi* api = static_cast<SubobjectApi*>(pointer);
            api->SetPrototype = SetPrototypeImpl;
            api->SetOwner = SetOwnerImpl;
            api->GetOwnedObjectsCount = GetOwnedObjectsCount;
            api->GetOwnedObjects = GetOwnedObjects;
            api->GetTypeId = GetTypeIdImpl;
            api->Remove = RemoveImpl;
        }

        static constexpr TypeID GetApiId()
        {
            return GetTypeID<SubobjectApi>();
        }
    };

    template <typename Type>
    SubobjectApi Subobject<Type>::GetApi()
    {
        SubobjectApi api{};
        TypeApiInfo<Subobject>::ExtractApi(&api);
        return api;
    }

    template <typename Type>
    class FY_API Value
    {
    public:
        Value& operator=(const Type& pValue)
        {
            hasValue = true;
            value = pValue;
            return *this;
        }

        template <typename Other>
        bool operator==(const Other& other) const
        {
            return Get() == other;
        }

        Type Get() const
        {
            if (hasValue)
            {
                return value;
            }

            if (prototype != nullptr)
            {
                return prototype->Get();
            }

            return {};
        }

        operator Type() const
        {
            return Get();
        }

        explicit operator bool() const
        {
            return hasValue;
        }

        friend class TypeApiInfo<Value>;

    private:
        bool   hasValue = false;
        Type   value = {};
        Value* prototype{};
    };

    struct ValueApi
    {
        void (*SetPrototype)(VoidPtr subobject, VoidPtr prototype);
    };

    template <typename Type>
    struct TypeApiInfo<Value<Type>>
    {
        static void SetPrototypeImpl(VoidPtr value, VoidPtr prototype)
        {
            static_cast<Value<Type>*>(value)->prototype = static_cast<Value<Type>*>(prototype);
        }

        static void ExtractApi(VoidPtr pointer)
        {
            ValueApi* api = static_cast<ValueApi*>(pointer);
            api->SetPrototype = SetPrototypeImpl;
        }

        static constexpr TypeID GetApiId()
        {
            return GetTypeID<ValueApi>();
        }
    };


    class FY_API Asset
    {
    public:
        virtual ~Asset() = default;

        virtual void Load() {}
        virtual void Unload() {}

        UUID GetUUID() const
        {
            return uuid;
        }

        void SetUUID(const UUID& p_uuid);

        Asset* GetPrototype() const
        {
            return prototype;
        }

        TypeHandler* GetAssetType() const
        {
            return assetType;
        }

        TypeID GetAssetTypeId() const
        {
            return assetType->GetTypeInfo().typeId;
        }

        StringView GetName() const
        {
            return name;
        }

        StringView GetPath() const
        {
            return path;
        }

        StringView GetAbsolutePath() const
        {
            return absolutePath;
        }

        void SetName(StringView p_name);

        bool IsActive() const
        {
            return active;
        }

        void SetActive(bool p_active);

        AssetDirectory* GetDirectory() const
        {
            return directory;
        }

        Asset* GetParent() const
        {
            if (subobjectOf)
            {
                return subobjectOf->GetOwner();
            }
            return nullptr;
        }

        bool IsChildOf(Asset* parent) const;

        virtual void BuildPath();
        virtual void OnActiveChanged() {}

        virtual void Modify()
        {
            currentVersion += 1;
        }

        bool IsModified() const
        {
            if (!IsActive() && loadedVersion == 0)
            {
                return false;
            }
            return currentVersion != loadedVersion;
        }

        virtual StringView GetDisplayName() const
        {
            if (assetType != nullptr)
            {
                return assetType->GetSimpleName();
            }
            return "Asset";
        }

        friend class AssetDatabase;

        template <typename Type>
        friend class Subobject;
        friend class AssetDirectory;

        static void RegisterType(NativeTypeHandler<Asset>& type);

    private:
        usize           index{};
        UUID            uuid{};
        String          path{};
        Asset*          prototype{};
        SubobjectBase*  subobjectOf{};
        TypeHandler*    assetType{};
        u64             currentVersion{};
        u64             loadedVersion{};
        String          name{};
        String          absolutePath{};
        AssetDirectory* directory{};
        bool            active = true;

        void ValidateName();
    };


    template<typename T>
    struct ArchiveType<Subobject<T>, Traits::EnableIf<Traits::IsBaseOf<Asset, T>>>
    {
        static void WriteField(ArchiveWriter& writer, ArchiveObject object, const StringView& name, const Subobject<T>& value)
        {
            TypeHandler* typeHandler = Registry::FindType<T>();
            ArchiveObject arr = writer.CreateArray();
            for (T* asset : value.GetOwnedObjects())
            {
                writer.AddValue(arr, typeHandler->Serialize(writer, asset));
            }
            writer.WriteValue(object, name, arr);
        }

        static Subobject<T> ReadField(ArchiveReader& reader, ArchiveObject object, const StringView& name)
        {
            return {};
        }
    };
}
