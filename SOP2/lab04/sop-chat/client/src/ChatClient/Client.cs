using System.IO;
using System.Net.Sockets;
using System.Text;
using System.Threading;

namespace ChatClient;

public class Client
{
        
    public const int NameOffset = 0;
    public const int NameSize = 64;
    public const int MessageOffset = NameSize;
    public const int MessageSize = 448;
    public const int BuffSize = NameSize + MessageSize;
    
    public string Name { get; set; } = "Unauthorized";
    public TcpClient TcpClient { get; }
    public bool Connected => TcpClient.Connected;
    public NetworkStream NetworkStream { get; }
    public StreamReader Reader { get; }
    public StreamWriter Writer { get; }
    public CancellationTokenSource Cancellation { get; } = new CancellationTokenSource();

    public Client(TcpClient tcpClient)
    {
        TcpClient = tcpClient;
        NetworkStream = TcpClient.GetStream();
        Reader = new StreamReader(NetworkStream, Encoding.ASCII);
        Writer = new StreamWriter(NetworkStream, Encoding.ASCII) { AutoFlush = true };
    }
}