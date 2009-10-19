The xml files in this directory provide the USB configuration details for the
test application t_usb_device.

For a description of the structure and usage of these files, see the
"Base How To Use Automated USB Tests" document
\generic\base\documentation\Base_How_To_Use_Automated_USB_Test.doc.

The files in this directory are as follows, along with a brief description of their use:-

For the H4 HRP  with the fibula high speed card running at full speed

fsif0.xml			Interface 0 - Default Setting, Endpoints Bulk IN and Bulk OUT	
fsif0a3.xml		Interface 0 - 3 Alternate Settings
fsif0a3if1a2if2.xml		Interface 0 - 3 Alternates, Interface 1 - 2 Alternates, Interface 2 - Default Setting

For the H2 HRP

h2if0.xml			Interface 0 - Default Setting, Endpoints Bulk IN and Bulk OUT	
h2if0a3.xml		Interface 0 - 3 Alternate Settings
h2if0a3if1a2if2.xml		Interface 0 - 3 Alternates, Interface 1 - 2 Alternates, Interface 2 - Default Setting

For the H4 HRP

h4if0.xml			Interface 0 - Default Setting, Endpoints Bulk IN and Bulk OUT	
h4if0a3.xml		Interface 0 - 3 Alternate Settings
h4if0a3if1a2if2.xml		Interface 0 - 3 Alternates, Interface 1 - 2 Alternates, Interface 2 - Default Setting
h4isoif0a3.xml		As h4if0a3.xml but with an Isochronous endpoint
h4isoif0a3if1a2if2.xml	As h4if0a3if1a2if2.xml but with an Isochronous endpoint and DMA set to Y			

For the H4 HRP with the fibula high speed card running at high speed

hsif0.xml			Interface 0 - Default Setting, Endpoints Bulk IN and Bulk OUT		
hsif0a3.xml		Interface 0 - 3 Alternate Settings
hsif0a3if1a2if2.xml		Interface 0 - 3 Alternates, Interface 1 - 2 Alternates, Interface 2 - Default Setting
hsif0a3dmaerr.xml		As hsif0a3.xml but with dma enabled		
hsif0dma.xml		As hsif0.xml but with dma enabled
pktszerr.xml		Used to reproduce defect DEF112114 that is now fixed

For each of the current hardware reference platforms which contain USB three configuration files are used. These
relete to the host test scripts in the following way:-

Configuration Files					Host Test Scripts

fsif0.xml, hsif0.xml, h2if0.xml, h4if0.xml			sanity.uts

fsif0a3.xml, hsif0a3.xml, h2if0a3.xml, h4if0a3.xml		singleif1.uts, singleif2.uts

fsif0a3if1a2if2.xml, hsif0a3if1a2if2.xml,			multif1.uts, multif2.uts
h2if0a3if1a2if2.xml, h4if0a3if1a2if2.xml

Filename		HRP		Interface		Alternate Setting	EndPoints

fsif0.xml	or	H4 with Fibula	0		0		1 - Bulk IN
hsifo.xml				0		0		2 - Bulk OUT

fsif0a3.xml or	H4 with Fibula	0		0		1 - Bulk IN
hsif0a3.xml			0		0		2 - Bulk OUT
				0		0		3 - Interrupt IN
				0		0		4 - Interrupt OUT
				0		0		5 - Bulk IN
				0		1		1 - Interrupt IN
				0		1		2 - Interrupt OUT
				0		1		3 - Bulk IN
				0		1		4 - Bulk OUT
				0		1		5 - Bulk OUT
				0		2		1 - Bulk IN
				0		2		2 - Bulk OUT
				0		2		3 - Bulk OUT
				0		2		4 - Interrupt IN
				0		2		5 – Isochronous OUT

fsif0a3if1a2if2.xml	H4 with Fibula	0		0		1 - Bulk IN
hsif0a3if1a2if2.xml			0		0		2 - Bulk OUT
				0		0		3 - Interrupt IN
				0		0		4 - Interrupt OUT
				0		0		5 - Bulk IN
				0		1		1 - Interrupt IN
				0		1		2 - Interrupt OUT
				0		1		3 - Bulk IN
				0		1		4 - Bulk OUT
				0		1		5 - Bulk OUT
				0		2		1 - Bulk IN
				0		2		2 - Bulk OUT
				0		2		3 - Bulk OUT
				0		2		4 - Interrupt IN
				0		2		5 - Isochronous OUT
				1		0		1 - Bulk IN
				1		0		2 - Bulk OUT
				1		1		1 - Bulk OUT
				1		1		2 - Interrupt IN
				2		0		1 - Interrupt IN
				2		0		2 - Bulk OUT

Filename		HRP		Interface		Alternate Setting	EndPoints

h2if0.xml		H2		0		0		1 - Bulk IN
				0		0		2 - Bulk OUT

h2if0a3.xml	H2		0		0		1 - Bulk IN
				0		0		2 - Bulk OUT
				0		0		3 - Interrupt IN
				0		0		4 - Interrupt OUT
				0		0		5 - Bulk IN
				0		1		1 - Interrupt IN
				0		1		2 - Interrupt OUT
				0		1		3 - Bulk IN
				0		1		4 - Bulk OUT
				0		1		5 - Bulk OUT
				0		2		1 - Bulk IN
				0		2		2 - Bulk OUT
				0		2		3 - Bulk OUT
				0		2		4 - Interrupt IN
				0		2		5 – Isochronous OUT

h2if0a3if1a2if2.xml	H2		0		0		1 - Bulk IN
				0		0		2 - Bulk OUT
				0		0		3 - Bulk IN
				0		0		4 - Bulk OUT
				0		0		5 - Bulk IN
				0		1		1 - Interrupt IN
				0		1		2 - Interrupt OUT
				0		1		3 - Bulk IN
				0		1		4 - Bulk OUT
				0		1		5 - Bulk OUT
				0		2		1 - Bulk IN
				0		2		2 - Bulk OUT
				0		2		3 - Bulk OUT
				0		2		4 - Interrupt IN
				0		2		5 - Isochronous OUT
				1		0		1 - Bulk IN
				1		0		2 - Bulk OUT
				1		1		1 - Bulk OUT
				1		1		2 - Interrupt IN
				2		0		1 - Interrupt IN
				2		0		2 - Bulk OUT

Filename		HRP		Interface		Alternate Setting	EndPoints

h4if0.xml		H4		0		0		1 - Bulk IN
				0		0		2 - Bulk OUT
h4if0a3.xml	H4		0		0		1 - Bulk IN
				0		0		2 - Bulk OUT
				0		0		3 - Bulk  IN
				0		0		4 - Bulk OUT
				0		0		5 - Bulk IN
				0		1		1 - Interrupt IN
				0		1		2 - Bulk OUT
				0		1		3 - Bulk IN
				0		1		4 - Bulk OUT
				0		1		5 - Bulk OUT
				0		2		1 - Bulk IN
				0		2		2 - Bulk OUT
				0		2		3 - Bulk OUT
				0		2		4 - Interrupt IN
				0		2		5 - Bulk OUT

h4if0a3if1a2if2.xml	H4		0		0		1 - Bulk IN
				0		0		2 - Bulk OUT
				0		0		3 - Bulk IN
				0		0		4 - Bulk OUT
				0		0		5 - Bulk IN
				0		1		1 - Interrupt IN
				0		1		2 - Bulk OUT
				0		1		3 - Bulk IN
				0		1		4 - Bulk OUT
				0		1		5 - Bulk OUT
				0		2		1 - Bulk IN
				0		2		2 - Bulk OUT
				0		2		3 - Bulk OUT
				0		2		4 - Interrupt IN
				0		2		5 - Bulk OUT
				1		0		1 - Bulk IN
				1		0		2 - Bulk OUT
				1		1		1 - Bulk OUT
				1		1		2 - Interrupt IN
				2		0		1 - Bulk IN
				2		0		2 - Bulk OUT