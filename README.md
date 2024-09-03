# BESFarm_control
실시간 센서 데이터를 바탕으로 제어 룰을 적용하여 구동기에 자동으로 명령을 내리는 지능형 제어 시스템 개발

## branch 설명
1. main <br><br>
2. controller 제어기  <br>
2.1 feature/sensor : 센서 데이터를 받아오는 과정 <br>
2.2 feature/rule_map : db에서 센서 데이터를 받아와서 제어룰에 따라 명령을 내리는 과정 (main.cpp + control.cpp + prepare_message.cpp + wirtenode.cpp) <br>
2.3 feature/act : 제어명령으로 레지스터 맵을 구성하고 구동기 노드로 내보내는 과정 <br><br>
3. actuator 구동기 노드
      
