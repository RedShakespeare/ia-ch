#ifndef ITEM_DEVICE_HPP
#define ITEM_DEVICE_HPP

#include <vector>
#include <string>

#include "item.hpp"

class Device: public Item
{
public:
        Device(ItemData* const item_data);

        virtual ~Device() {}

        virtual ConsumeItem activate(Actor* const actor) override = 0;

        Color interface_color() const override final
        {
                return colors::cyan();
        }

        virtual void on_std_turn_in_inv(const InvType inv_type) override
        {
                (void)inv_type;
        }

        void identify(const Verbosity verbosity) override;
};

class StrangeDevice : public Device
{
public:
        StrangeDevice(ItemData* const item_data);

        virtual std::vector<std::string> descr() const override final;

        ConsumeItem activate(Actor* const actor) override;

        virtual std::string name_inf_str() const override;

        virtual void save() override;
        virtual void load() override;

        Condition condition_;

private:
        virtual std::string descr_identified() const = 0;

        virtual ConsumeItem run_effect() = 0;
};

class DeviceBlaster : public StrangeDevice
{
public:
        DeviceBlaster(ItemData* const item_data) :
                StrangeDevice(item_data) {}

        ~DeviceBlaster() override {}

private:
        std::string descr_identified() const override
        {
                return
                        "When activated, this device blasts all visible "
                        "enemies with infernal power.";
        }

        ConsumeItem run_effect() override;
};

class DeviceRejuvenator : public StrangeDevice
{
public:
        DeviceRejuvenator(ItemData* const item_data) :
                StrangeDevice(item_data) {}

        ~DeviceRejuvenator() override {}

private:
        std::string descr_identified() const override
        {
                return
                        "When activated, this device heals all wounds and "
                        "physical maladies. The procedure is very painful and "
                        "invasive however, and causes great shock to the user.";
        }

        ConsumeItem run_effect() override;
};

class DeviceTranslocator : public StrangeDevice
{
public:
        DeviceTranslocator(ItemData* const item_data) :
                StrangeDevice(item_data) {}

        ~DeviceTranslocator() override {}

private:
        std::string descr_identified() const override
        {
                return
                        "When activated, this device teleports all visible "
                        "enemies to different locations.";
        }

        ConsumeItem run_effect() override;
};

class DeviceSentryDrone : public StrangeDevice
{
public:
        DeviceSentryDrone(ItemData* const item_data) :
                StrangeDevice(item_data) {}

        ~DeviceSentryDrone() override {}

private:
        std::string descr_identified() const override
        {
                return
                        "When activated, this device will \"come alive\" and "
                        "guard the user.";
        }

        ConsumeItem run_effect() override;
};

class DeviceDeafening : public StrangeDevice
{
public:
        DeviceDeafening(ItemData* const item_data) :
                StrangeDevice(item_data) {}

        ~DeviceDeafening() override {}

private:
        std::string descr_identified() const override
        {
                return
                        "When activated, this device causes temporary deafness "
                        "in all creatures in a large area (on the whole map), "
                        "except for the user.";
        }

        ConsumeItem run_effect() override;
};

class DeviceLantern : public Device
{
public:
        DeviceLantern(ItemData* const item_data);

        ~DeviceLantern() override {}

        std::string name_inf_str() const override;

        ConsumeItem activate(Actor* const actor) override;

        void on_std_turn_in_inv(const InvType inv_type) override;

        void on_pickup_hook() override;

        LgtSize lgt_size() const override;

        void save() override;
        void load() override;

        int nr_turns_left_;
        bool is_activated_;

private:
        void toggle();
};

#endif // ITEM_DEVICE_HPP
