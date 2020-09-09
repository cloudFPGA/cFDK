

## Part-2: Development Kit for cloudFPGA

### Step-1: Fetch the cloudFPGA Development Kit
Clone the git repository located at:

![image](images/toDo_red.PNG)
[TODO: The cFDK will become Open Source at GitHub]

Until available, use “SRA-r0.2.zip” and extract it in your VM.

### Step-2: Test of the Implementation Flow
TopFlash → Project used to create the content of the Flash

```
cd ./SRA/FMKU60/TOP/TopFlash
make monolithic
```

![image](images/cf-EndOfBuild.PNG)

## Part-3: Upload Image, Request and Program FPGA

### Step-1: Access cloudFPGA Resource Manager
Point your browser at [http://10.12.0.132:8080/ui](http://10.12.0.132:8080/ui)
and expand the “Images” section

![image](images/cf-Resource_Manager.PNG)

### Step-2: Upload an Image

#### Step-2a: Expand the “POST” operation

![image](images/cf-RM_Images.PNG)

#### Step-2b: Fill in the parameter fields

![image](images/cf-RM_POST_Image.PNG)

#### Step-2c: Check the “Response Body”

![image](images/cf-RM_POST_Image_Response.PNG)

### Step-3: Request and Program FPGA Instance

#### Step-3a: Expand the “Instance” section

![image](images/cf-Resource_Manager_API.PNG)

#### Step-3b: Expand the “POST” operation

![image](images/cf-RM_Instances.PNG)

#### Step-3c: Fill in the parameter fields

And press the “Try it out!” button

![image](images/cf-RM_POST_Instance.PNG)

#### Step-3d: Check the “Response Body”

![image](images/cf-RM_POST_Instance_Response.PNG)

### Step-4: Ping the IP Address of the FPGA

![image](images/cf-Ping.PNG)


## Part-4: Remote Debugging with Vivado

### Step-1: Request Connection to HW Server

#### Step-1a: Point your browser at [http://10.12.0.132:8080/ui](http://10.12.0.132:8080/ui)
and expand the “Debug” section

![image](images/cf-Resource_Manager_Debug.PNG)

#### Step-1b: Expand the “Get /debug/ila_connection/{instance_id}” operation

![image](images/cf-RM_Debug.PNG)

#### Step-1c: Fill in the parameter fields

Press the “Try it out!” button

![image](images/cf-RM_Get_ILAD_Connection.PNG)

#### Step-1d: Check the “Response Body”

![image](images/cf-RM_Get_ILAD_Connection_Response.PNG)

### Step-2: Launch Vivado
The Xilinx installation path on the ZYC2 VM:
`/tools/Xilinx/Vivado/2017.4/`

![image](images/cf-Launch_Vivado_HW_Manager.PNG)

### Step-3:

#### Step-3a: Open the Hardware Server

![image](images/cf-OpenNewVivadoTarget_1.PNG)

#### Step-3b: Select your Hardware Target

![image](images/cf-Open_NewVivadoTarget_2.PNG)

#### Step-3c: Here you go...

![image](images/cf-Vivado_Hardware_Manager_Window.PNG)

### Step-4: Further Instance Operations

![image](images/cf-RM_Instances_Operations.png)

## Backup

### HowTo: “Hello World”
TopEchoHls (is a kind of 'Hello World')
```
cd ./SRA/FMKU60/TOP/TopEchoHls
make monolithic
```

![image](images/cf-HelloWorld_underConstruction.png)

### HowTo: Test the UDP Loopback w/ netcat

![image](images/cf-Netcat.PNG)
