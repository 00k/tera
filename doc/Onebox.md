Ϊ������ٶ�����Tera, Tera�ṩһ��OneBox��ʽ�������ñ��ش洢����DFS���������������master��tabletnode����ģʽ�¿�����ʹ��tera��ȫ�����ܣ������������ڣ���
* ����½������롢ж�ء�schema�޸ġ�ɾ����
* ����д�롢ɾ�����������˳����ȡ���汾���Ƶ�
* ���ݷ�Ƭ���ѡ��ϲ������ؾ����

## ����֮ǰ��
Ҫ����������������
* **Tera����ͨ��**��������tera_main, teracli�����������ļ�Ϊ׼��
* **ӵ��һ��zookeeper����**���������ɣ�

## ׼������
1. ��zk�ϴ���һ��Tera���ڵ㣬��������4���ӽڵ㣺**root_table��master-lock��ts��kick**���ɹ�������ͼ��ʾ��
2. ���������ɵ�tera_main, teracli�����������ļ�����example/onebox/bin�£���Ŀ¼��Ӧ�������ļ���
3. �޸������ļ�example/onebox/conf/tera.flag�������Ľ�����zk�ڵ���Ϣ�����Ӧ����������ʱ���䡣

## ������ֹͣTera
* ִ��example/onebox/bin/launch_tera.sh��������Tera����ͨ��config��ѡ������tabletnode������
* ִ��example/onebox/bin/kill_tera.sh����ֹͣTera

## ���鿪ʼ��
### ����ͨ��teracli����������Tera
�����������������ִ��:

`./teracli show`

�������ͼ������ʾ�����ɹ�

### �о�Щ��������
�鿴����ϸ�ı����Ϣ��

`./teracli showx`

�鿴��ǰ����Щ����tabletnode:

`./teracli showts`

�鿴����ϸ��tabletnode��Ϣ��

`./teracli showtsx`

�½�һ�����hello:

`./teracli create hello`

д��һ�����ݣ�

`./teracli put hello row_first ":" value`

����һ�����ݣ�

`./teracli get hello row_first ":"`

ж�ر��

`./teracli disable hello`

ɾ�����

`./teracli drop hello`

## д�����
Tera oneboxģʽ�������鼸�����еĹ������ԣ�ϣ��ͨ������Ľ��ܿ����ô�Ҷ�Tera��һ����������ʶ��
