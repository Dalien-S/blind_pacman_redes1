# Client:

- `sendMovement()`
- `waitForServer()`
```c
game_is_running = true;
do {
    PacketType type = receive(socket, &buffer);

    switch(type) {
        case file:
            // TODO (std::string receiveFile(socket, type, &buffer))
            // receiveFile abre um arquivo com o nome no buffer ++ (_received.tipo)
            // limpa o buffer
            // manda uma mensagem dummy para o server passar para a próxima mensagem (os dados)
            // recebe os dados 
            // escreve os dados no arquivo
            receiveFile(socket, type, &buffer); // TODO (abre arquivo com o que)
            // TODO
            // abre o arquivo (fork() -> execve("xdg-open", nome_do_arquivo))
            openFile();
            break;
        case grid:
            // TODO
            // vai escrever o grid no buffer
            receiveGrid(socket, &buffer); // TODO
            printGrid();
            break;
        case move_req:
            sendMovement(socket); // TODO
            break;
        case end_transmission:
            endGame(buffer); // TODO
            game_is_running = false;
            break;
        default:
    }
} while (game_is_running);
```

# Server: 

```cpp

void requestForMove(int socket) {
    send(socket, req_move);
    confirmSend(socket);
}

void sendGrid(int socket, GameState* game) {
    const char* grid = game->generateGrid(int);

    send(socket, visualization, NULL);
    confirmSend(socket);

    receive(socket); // dummy

    send(socket, data, grid);
    confirmSend(socket);
}

vector<char>* readEntireFile(const char* filepath) {
    int size = "read"(filepath);

    vector<char>* a = new vector<char>();
    a.insert(file);

    return a;
}

// carregar arquivo
// inicializar o jogo
bool game_running = true;
do {
    sendGrid(socket, game);
    receive(socket); // dummy
    requestForMove(socket);

    type = receive(socket, &buffer);

    switch(type) {
        case move:
            status = game->update(getDir(&buffer));
            break;
        default:
            logger.print("não recebeu o que deveria\n");
    }

    switch(status) {
        case FILE:
            file = readEntireFile();
            send(socket, tipo, file->data());
            confirmSend(socket);
            break;
        case win:
            // ... sendWin();
            game_running = false;
            break;
        case lose:
            // ... sendLose();
            game_running = false;
            break;
    }

} while(game_running);
```



```c
int main()

```
