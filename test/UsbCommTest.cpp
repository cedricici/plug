/*
 * PLUG - software to operate Fender Mustang amplifier
 *        Linux replacement for Fender FUSE software
 *
 * Copyright (C) 2017-2018  offa
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "com/UsbComm.h"
#include "com/CommunicationException.h"
#include "mocks/LibUsbMocks.h"
#include "helper/Matcher.h"
#include <vector>
#include <array>
#include <gmock/gmock.h>

using plug::com::CommunicationException;
using plug::com::UsbComm;
using namespace testing;
using namespace test::matcher;

class UsbCommTest : public testing::Test
{
protected:
    void SetUp() override
    {
        comm = std::make_unique<UsbComm>();
        usbmock = mock::resetUsbMock();
    }

    void TearDown() override
    {
        mock::clearUsbMock();
    }

    void setupHandle()
    {
        EXPECT_CALL(*usbmock, init(_));
        EXPECT_CALL(*usbmock, open_device_with_vid_pid(_, _, _)).WillOnce(Return(&handle));
        EXPECT_CALL(*usbmock, kernel_driver_active(_, _));
        EXPECT_CALL(*usbmock, claim_interface(_, _));
        comm->open(0, 0);
    }

    std::unique_ptr<UsbComm> comm;
    mock::UsbMock* usbmock;
    libusb_device_handle handle{};
    static inline constexpr std::uint16_t vid{7};
    static inline constexpr std::uint16_t pid{9};
    static inline constexpr int failed{17};
    static inline constexpr std::uint16_t timeout{500};
};

TEST_F(UsbCommTest, openOpensConnection)
{
    InSequence s;
    EXPECT_CALL(*usbmock, init(nullptr));
    EXPECT_CALL(*usbmock, open_device_with_vid_pid(nullptr, vid, pid)).WillOnce(Return(&handle));
    EXPECT_CALL(*usbmock, kernel_driver_active(&handle, 0));
    EXPECT_CALL(*usbmock, claim_interface(&handle, 0));

    comm->open(vid, pid);
    EXPECT_THAT(comm->isOpen(), Eq(true));
}

TEST_F(UsbCommTest, openThrowsIfOpenFails)
{
    InSequence s;
    EXPECT_CALL(*usbmock, init(_));
    EXPECT_CALL(*usbmock, open_device_with_vid_pid(_, _, _)).WillOnce(Return(nullptr));

    EXPECT_THROW(comm->open(vid, pid), CommunicationException);
}

TEST_F(UsbCommTest, openDetachesDriverIfNotActive)
{
    InSequence s;
    EXPECT_CALL(*usbmock, init(_));
    EXPECT_CALL(*usbmock, open_device_with_vid_pid(_, _, _)).WillOnce(Return(&handle));
    EXPECT_CALL(*usbmock, kernel_driver_active(_, _)).WillOnce(Return(failed));
    EXPECT_CALL(*usbmock, detach_kernel_driver(&handle, 0));
    EXPECT_CALL(*usbmock, claim_interface(_, _));

    comm->open(vid, pid);
}

TEST_F(UsbCommTest, openThrowsIfDetachingDriverFailed)
{
    InSequence s;
    EXPECT_CALL(*usbmock, init(_));
    EXPECT_CALL(*usbmock, open_device_with_vid_pid(_, _, _)).WillOnce(Return(&handle));
    EXPECT_CALL(*usbmock, kernel_driver_active(_, _)).WillOnce(Return(failed));
    EXPECT_CALL(*usbmock, detach_kernel_driver(_, _)).WillOnce(Return(failed));

    EXPECT_THROW(comm->open(vid, pid), CommunicationException);
}

TEST_F(UsbCommTest, openThrowsIfClaimingInterfaceFailed)
{
    InSequence s;
    EXPECT_CALL(*usbmock, init(_));
    EXPECT_CALL(*usbmock, open_device_with_vid_pid(_, _, _)).WillOnce(Return(&handle));
    EXPECT_CALL(*usbmock, kernel_driver_active(_, _));
    EXPECT_CALL(*usbmock, claim_interface(_, _)).WillOnce(Return(failed));

    EXPECT_THROW(comm->open(vid, pid), CommunicationException);
}

TEST_F(UsbCommTest, openFirstOpensFirstConnectionAvailable)
{
    InSequence s;
    EXPECT_CALL(*usbmock, init(nullptr));
    EXPECT_CALL(*usbmock, open_device_with_vid_pid(nullptr, vid, _))
        .Times(2)
        .WillRepeatedly(Return(nullptr));
    EXPECT_CALL(*usbmock, open_device_with_vid_pid(nullptr, vid, pid)).WillOnce(Return(&handle));
    EXPECT_CALL(*usbmock, kernel_driver_active(&handle, 0));
    EXPECT_CALL(*usbmock, claim_interface(&handle, 0));

    comm->openFirst(vid, {18, 17, pid, 3});
    EXPECT_THAT(comm->isOpen(), Eq(true));
}

TEST_F(UsbCommTest, openFirstThrowsIfOpenFails)
{
    InSequence s;
    EXPECT_CALL(*usbmock, init(_));
    EXPECT_CALL(*usbmock, open_device_with_vid_pid(_, _, _)).WillOnce(Return(nullptr));

    EXPECT_THROW(comm->openFirst(vid, {pid}), CommunicationException);
}

TEST_F(UsbCommTest, openFirstDetachesDriverIfNotActive)
{
    InSequence s;
    EXPECT_CALL(*usbmock, init(_));
    EXPECT_CALL(*usbmock, open_device_with_vid_pid(_, _, _)).WillOnce(Return(&handle));
    EXPECT_CALL(*usbmock, kernel_driver_active(_, _)).WillOnce(Return(failed));
    EXPECT_CALL(*usbmock, detach_kernel_driver(&handle, 0));
    EXPECT_CALL(*usbmock, claim_interface(_, _));

    comm->openFirst(vid, {pid});
}

TEST_F(UsbCommTest, openFirstThrowsIfDetachingDriverFailed)
{
    InSequence s;
    EXPECT_CALL(*usbmock, init(_));
    EXPECT_CALL(*usbmock, open_device_with_vid_pid(_, _, _)).WillOnce(Return(&handle));
    EXPECT_CALL(*usbmock, kernel_driver_active(_, _)).WillOnce(Return(failed));
    EXPECT_CALL(*usbmock, detach_kernel_driver(_, _)).WillOnce(Return(failed));

    EXPECT_THROW(comm->openFirst(vid, {pid}), CommunicationException);
}

TEST_F(UsbCommTest, openFirstThrowsIfClaimingInterfaceFailed)
{
    InSequence s;
    EXPECT_CALL(*usbmock, init(_));
    EXPECT_CALL(*usbmock, open_device_with_vid_pid(_, _, _)).WillOnce(Return(&handle));
    EXPECT_CALL(*usbmock, kernel_driver_active(_, _));
    EXPECT_CALL(*usbmock, claim_interface(_, _)).WillOnce(Return(failed));

    EXPECT_THROW(comm->openFirst(vid, {pid}), CommunicationException);
}

TEST_F(UsbCommTest, closeClosesConnection)
{
    setupHandle();

    InSequence s;
    EXPECT_CALL(*usbmock, release_interface(&handle, 0));
    EXPECT_CALL(*usbmock, attach_kernel_driver(&handle, 0));
    EXPECT_CALL(*usbmock, close(&handle));
    EXPECT_CALL(*usbmock, exit(nullptr));

    comm->close();
    EXPECT_THAT(comm->isOpen(), Eq(false));
}

TEST_F(UsbCommTest, closeDoesNothingIfNotOpenYet)
{
    comm->close();
    EXPECT_THAT(comm->isOpen(), Eq(false));
}

TEST_F(UsbCommTest, closeDoesNotReattachDriverIfNoDevice)
{
    setupHandle();

    InSequence s;
    EXPECT_CALL(*usbmock, release_interface(&handle, 0)).WillOnce(Return(LIBUSB_ERROR_NO_DEVICE));
    EXPECT_CALL(*usbmock, close(&handle));
    EXPECT_CALL(*usbmock, exit(nullptr));

    comm->close();
}

TEST_F(UsbCommTest, interruptWriteTransfersDataVector)
{
    setupHandle();

    const std::vector<std::uint8_t> data{0, 1, 2, 3, 4, 5, 6};
    constexpr std::uint8_t endpoint{0x81};

    InSequence s;
    EXPECT_CALL(*usbmock, interrupt_transfer(&handle, endpoint, BufferIs(data), data.size(), _, timeout))
        .WillOnce(DoAll(SetArgPointee<4>(data.size()), Return(0)));

    const auto n = comm->interruptWrite(endpoint, data);
    EXPECT_THAT(n, Eq(data.size()));
}

TEST_F(UsbCommTest, interruptWriteTransfersDataArray)
{
    setupHandle();

    const std::array<std::uint8_t, 4> data{{0, 1, 2, 3}};
    constexpr std::uint8_t endpoint{0x81};

    InSequence s;
    EXPECT_CALL(*usbmock, interrupt_transfer(&handle, endpoint, BufferIs(data), data.size(), _, timeout))
        .WillOnce(DoAll(SetArgPointee<4>(data.size()), Return(0)));

    const auto n = comm->interruptWrite(endpoint, data);
    EXPECT_THAT(n, Eq(data.size()));
}

TEST_F(UsbCommTest, interruptWriteReturnsActualWrittenOnPartialTransfer)
{
    setupHandle();

    const std::vector<std::uint8_t> data{0, 1, 2, 3, 4, 5, 6};
    const std::vector<std::uint8_t> partial(data.cbegin(), std::next(data.cbegin(), 4));
    constexpr std::uint8_t endpoint{0x81};

    InSequence s;
    EXPECT_CALL(*usbmock, interrupt_transfer(&handle, endpoint, BufferIs(data), data.size(), _, timeout))
        .WillOnce(DoAll(SetArgPointee<4>(partial.size()), Return(0)));

    const auto n = comm->interruptWrite(endpoint, data);
    EXPECT_THAT(n, Eq(partial.size()));
}

TEST_F(UsbCommTest, interruptReadReceivesData)
{
    setupHandle();

    const std::vector<std::uint8_t> data{0, 1, 2, 3, 4, 5, 6};
    const std::size_t readSize = data.size();
    constexpr std::uint8_t endpoint{0x01};

    InSequence s;
    EXPECT_CALL(*usbmock, interrupt_transfer(&handle, endpoint, _, data.size(), _, timeout))
        .WillOnce(DoAll(SetArrayArgument<2>(data.cbegin(), data.cend()), SetArgPointee<4>(readSize), Return(0)));

    const auto buffer = comm->interruptReceive(endpoint, readSize);
    EXPECT_THAT(buffer, ContainerEq(data));
}

TEST_F(UsbCommTest, interruptReadResizesBufferOnPartialTransfer)
{
    setupHandle();

    const std::vector<std::uint8_t> data{0, 1, 2, 3, 4, 5, 6};
    const std::vector<std::uint8_t> expected{0, 1, 2, 3};
    const std::size_t readSize = data.size();
    constexpr std::size_t actualSize{4};
    constexpr std::uint8_t endpoint{0x81};

    InSequence s;
    EXPECT_CALL(*usbmock, interrupt_transfer(&handle, endpoint, _, data.size(), _, timeout))
        .WillOnce(DoAll(SetArrayArgument<2>(data.cbegin(), std::next(data.cbegin(), actualSize)), SetArgPointee<4>(actualSize), Return(0)));

    const auto buffer = comm->interruptReceive(endpoint, readSize);
    EXPECT_THAT(buffer, ContainerEq(expected));
}
