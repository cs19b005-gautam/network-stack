# Ethernet
## Intro:
- LAN is a data communication network connecting various terminals or computers within a building or a limited geographical area.
- The connection between the devices could be wired or wireless.
<br></br>
- Examples of standard LAN technologies are:
    - <strong>Ethernet</strong>
    - Token Ring
    - Wireless LAN
<hr/>  

## Topologies of LAN
- Star Topology
- Bus Topology
- Ring Topology
- Mesh Topology
- Hybrid Topology
- Tree Topology
<hr/>  

## Ethernet Definition:
- Most widely used LAN technology, defined ubnder IEEE standards
- Reason behind the wide usability is Ethernet is easy to understand, implement, maintain and allows low-cost network implementation
- Offers flexibility in terms of topologies that are allowed.
- Generally uses BUS TOPOLOGY in the two layers of the OSI model (Physical layer and Data-Link Layer)
- Protocol data unit : Frame since we mainly deal with DLL.
- Access Mechanism used : <strong>CSMA/CD</strong>
<hr/>
<hr/>
<br></br>
<br></br>

# Ethernet Header
## 0. Introduction:
- Basic format used for all MAC implementation 
- Ethernet Frame consists of:
    <ol>
    <li> Preamble </li>
    <li> SFD (Start of frame delimiter)</li>
    <li> Destination Address</li>
    <li> Source Address</li>
    <li> Length</li>
    <li> Data</li>
    <li> CRC (Cyclic Redundancy Check)</li>
    </ol>

## 1. Preamble (PRE):
- Size : 7 bytes
- This is a pattern of alternative 0s and 1s, which indicates starting of the frame and allow sender and receiver to establish bit synchronization.
- Initially introduced to allow for the loss of a few bits due to allow for a loss of a few bits due to signal delays.
 
## 2. Start of Frame Delimiter (SFD):
- <strong>1-Byte field</strong> always set to 10101011.
- SFD indicates that upcoming bits are starting of the frame, which is destination address
- Sometimes SFD is a part of PRE
- SFD warns thee station(s) that this is last chance of synchronization

## 3. Destination Address:
- Size : 6-Byte
- Contains MAC address of a machine for which the data is to be destined

## 4. Source Address: 
- Size : 6-Byte
- Contains MAC address of source machine
- Source address is always an individual address (Unicast), the least significant bit of first byte is always 0.

## 5. Length:
- Size : 2-Byte field, indicates length of total Ethernet Frame
- Even though the maximum length that can be stored here is 65,534 length should not be greater than 1500

## 6. Data/Payload:
- This is the place where actual data is inserted here if Internet Protocol is used over Ethernet.
- Maximum data present can be as long as 1500 Bytes

## 7. Cyclic Redundancy Check:
- Size : 4 Bytes
- Contains 32-bit hash code of data, which is generated over the Destination Address, Source Address, Length, and Data Field.
- If check-sum computed by the destination is not as same as sent checkSum value, data received is corrupted.
