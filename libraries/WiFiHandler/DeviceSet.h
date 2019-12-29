#ifndef __DeviceSet__
#define __DeviceSet__

class DeviceSet {
	public:

	DeviceSet() {
		
	}

	virtual unsigned int length() = 0;
	virtual Configurable* configurable(unsigned int index) = 0;
	// virtual void dataReceived(CDElement* data) = 0; // TODO:: get rid of this
};

#endif
