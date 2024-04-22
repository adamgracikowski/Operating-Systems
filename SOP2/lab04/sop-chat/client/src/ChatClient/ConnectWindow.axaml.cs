using System;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading.Tasks;
using Avalonia.Controls;
using Avalonia.Input;
using Avalonia.Interactivity;
using Avalonia.Threading;
using MsBox.Avalonia;
using MsBox.Avalonia.Enums;

namespace ChatClient;

public partial class ConnectWindow : Window
{
    public Client? Client { get; private set; }
    public string Address { get; set; } = "localhost";
    public string Port { get; set; } = "9000";
    public string Username { get; set; } = "User";
    public string Key { get; set; } = "";

    public ConnectWindow()
    {
        InitializeComponent();
        DataContext = this;
    }

    private void OnEnter(object? sender, KeyEventArgs e)
    {
        if(e is { Key: Avalonia.Input.Key.Enter } && ConnectButton.IsEnabled)
            OnConnect(sender, e);
    }

    private async void OnConnect(object? sender, RoutedEventArgs e)
    {
        ProgressBar.IsIndeterminate = true;
        ConnectButton.IsEnabled = false;
        await Task.Run(Connect).ContinueWith(async task =>
        {
            if (task.IsCompletedSuccessfully)
            {
                await Dispatcher.UIThread.InvokeAsync(async () =>
                {
                    ProgressBar.IsIndeterminate = false;
                    await MessageBoxManager.GetMessageBoxStandard(
                            "Connected", 
                            "Connected successfully", 
                            ButtonEnum.Ok,
                            MsBox.Avalonia.Enums.Icon.Success, 
                            WindowStartupLocation.CenterOwner)
                        .ShowWindowDialogAsync(this);
                    Close();
                });
            }
            else
            {
                await Dispatcher.UIThread.InvokeAsync(async () =>
                    {
                        await MessageBoxManager.GetMessageBoxStandard(
                                "Failed", 
                                $"Failed to connect: {task.Exception?.Message}", 
                                ButtonEnum.Ok,
                                MsBox.Avalonia.Enums.Icon.Error, 
                                WindowStartupLocation.CenterOwner)
                            .ShowWindowDialogAsync(this);
                        ConnectButton.IsEnabled = true;
                        ProgressBar.IsIndeterminate = false;
                    });
            }
            
        });
    }

    private void Connect()
    {
        var address = Address.Trim();
        var portText = Port.Trim();
        var host = Dns.GetHostEntry(address);
        var port = ushort.Parse(portText);
        var username = Username.Trim();
        var key = Key;
        
        var ip = Array.Find(host.AddressList, addr => addr.AddressFamily == AddressFamily.InterNetwork);
        if (ip == null) throw new Exception("Failed to resolve address");
        
        var tcpClient = new TcpClient();
        tcpClient.Connect(ip, port);
        
        var client = new Client(tcpClient) { Name = username };
        var buffer = new byte[Client.BuffSize];
        
        Encoding.ASCII.GetBytes(username, 0, Math.Min(Client.NameSize, username.Length), buffer, Client.NameOffset);
        Encoding.ASCII.GetBytes(key, 0, Math.Min(Client.MessageSize, key.Length), buffer, Client.MessageOffset);
        client.NetworkStream.Write(buffer, 0, Client.BuffSize);
        client.NetworkStream.Flush();
        client.NetworkStream.ReadExactly(buffer);

        Client = client;
    }
}