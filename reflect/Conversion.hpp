#ifndef CONVERSION_H
#define CONVERSION_H

#include <any>

#include "TypeDescriptor.hpp"

namespace Reflect
{

	class Conversion
	{
	public:
		const TypeDescriptor *GetFromType() const { return mFromType; }
		const TypeDescriptor *GetToType() const { return mToType; }

		virtual std::any Convert(const void *object) const = 0;

	protected:
		Conversion(const TypeDescriptor *from, const TypeDescriptor *to)
			: mFromType(from), mToType(to) {}

	private:
		const TypeDescriptor *mFromType;  // type to convert from
		const TypeDescriptor *mToType;    // type to convert to
	};

	template <typename From, typename To>
	class ConversionImpl : public Conversion
	{
	public:
		ConversionImpl() : Conversion(Details::Resolve<From>(), Details::Resolve<To>()) {}

		std::any Convert(const void *object) const override
		{
			//return To(*static_cast<const From*>(object));
			return static_cast<To>(*static_cast<const From*>(object));
		}
	};

}

#endif // CONVERSION_H