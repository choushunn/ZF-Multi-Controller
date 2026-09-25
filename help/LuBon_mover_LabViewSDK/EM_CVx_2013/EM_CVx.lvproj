<?xml version='1.0' encoding='UTF-8'?>
<Project Type="Project" LVVersion="13008000">
	<Item Name="我的电脑" Type="My Computer">
		<Property Name="server.app.propertiesEnabled" Type="Bool">true</Property>
		<Property Name="server.control.propertiesEnabled" Type="Bool">true</Property>
		<Property Name="server.tcp.enabled" Type="Bool">false</Property>
		<Property Name="server.tcp.port" Type="Int">0</Property>
		<Property Name="server.tcp.serviceName" Type="Str">我的电脑/VI服务器</Property>
		<Property Name="server.tcp.serviceName.default" Type="Str">我的电脑/VI服务器</Property>
		<Property Name="server.vi.callsEnabled" Type="Bool">true</Property>
		<Property Name="server.vi.propertiesEnabled" Type="Bool">true</Property>
		<Property Name="specify.custom.address" Type="Bool">false</Property>
		<Item Name="example" Type="Folder">
			<Item Name="EM_CVx_example.vi" Type="VI" URL="../example/EM_CVx_example.vi"/>
		</Item>
		<Item Name="library" Type="Folder">
			<Item Name="moverLibrary_x64.dll" Type="Document" URL="../library/moverLibrary_x64.dll"/>
			<Item Name="moverLibrary_x86.dll" Type="Document" URL="../library/moverLibrary_x86.dll"/>
		</Item>
		<Item Name="EM_CVx.lvlib" Type="Library" URL="../EM_CVx.lvlib"/>
		<Item Name="依赖关系" Type="Dependencies"/>
		<Item Name="程序生成规范" Type="Build"/>
	</Item>
</Project>
