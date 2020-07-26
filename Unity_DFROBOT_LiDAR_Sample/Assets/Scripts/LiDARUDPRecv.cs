using System.Collections;
using System.Collections.Generic;
using System.Net;
using System.Net.Sockets;
using System.Net.NetworkInformation;
using System.Threading;
using UnityEngine;

public class LiDARUDPRecv : MonoBehaviour
{
    static int NUM_POS = 360;

    internal UdpClient m_receiveUdp = null;
    internal Thread m_thread = null;
    [SerializeField] internal int m_receivePort = 7003;

    float[] m_dirArr;
    Vector3[] m_posArr;
    GameObject[] m_pointGoArr;

    // Start is called before the first frame update
    void Start()
    {
        m_dirArr = new float[NUM_POS];
        m_posArr = new Vector3[NUM_POS];

        for (int i=0;i<m_dirArr.Length;++i)
            m_dirArr[i] = 5000f; // [mm]

        getPosArr();
        m_pointGoArr = new GameObject[NUM_POS];
        for (int i = 0; i < m_pointGoArr.Length; ++i)
        {
            m_pointGoArr[i] = GameObject.CreatePrimitive(PrimitiveType.Quad);
            m_pointGoArr[i].transform.position = m_posArr[i];
            m_pointGoArr[i].transform.localScale = Vector3.one * 0.5f;
            m_pointGoArr[i].transform.localRotation = Quaternion.AngleAxis(-90f, Vector3.left);
        }


        m_receiveUdp = new UdpClient(m_receivePort);
        m_receiveUdp.Client.ReceiveTimeout = 1000;
        m_thread = new Thread(new ThreadStart(ThreadMethod));
        m_thread.Start();

    }

    // Update is called once per frame
    void Update()
    {
        lock (m_thread)
        { // m_thread.lock
            getPosArr();
            for (int i = 0; i < m_pointGoArr.Length; ++i)
            {
                m_pointGoArr[i].transform.position = m_posArr[i];
            }
        }
    }

    void getPosArr()
    {
        for (int i = 0; i < m_dirArr.Length; ++i)
        {
            float dist = m_dirArr[i];
            float ang = 360f * (float)i / (float)NUM_POS;
            m_posArr[i % NUM_POS] = Quaternion.AngleAxis(ang, Vector3.up) * Vector3.forward * dist * 0.001f; // [mm]->[m]
        }
    }

    internal virtual void OnApplicationQuit()
    {
        udpStop();
    }

    internal virtual void OnDestroy()
    {
        udpStop();
    }

    private void udpStop()
    {
        if (m_thread != null)
        {
            m_thread.Abort();
            m_thread = null;
            Debug.Log("UDPServerThreadStopped");
        }
        if (m_receiveUdp != null)
        {
            m_receiveUdp.Close();
            m_receiveUdp = null;
        }
    }

//34:A: 38.58, D: 03123.00, Q: 47, S:  ,

    private void ThreadMethod()
    {
        while (true)
        {
            if (m_receiveUdp.Available > 0)
            {
                try
                {
                    IPEndPoint remoteEP = null;
                    byte[] data = m_receiveUdp.Receive(ref remoteEP);
                    string text = System.Text.Encoding.UTF8.GetString(data);
                    string[] dataArr = text.Split(',');
                    if (dataArr.Length > 2)
                    {
                        string[] angArr = dataArr[0].Split(':');
                        string[] distArr = dataArr[1].Split(':');
                        string[] qArr = dataArr[2].Split(':');
                        float ang = 0f;
                        int dist = 0;
                        int q = 0;
                        float.TryParse(angArr[1], out ang);
                        int.TryParse(distArr[1], out dist);
                        int.TryParse(qArr[1], out q);
                        if (q > 0)
                        {
                            int iAng = Mathf.RoundToInt(ang * ((float)NUM_POS / 360f)) % NUM_POS;
                            m_dirArr[iAng] = dist;
                            //Debug.Log(iAng + ":" + m_dirArr[iAng] + ":" + q);
                        }
                    }
                }
                catch (SocketException e)
                {
                    Debug.Log(e.ToString());
                }
            }
        }
    }
}
