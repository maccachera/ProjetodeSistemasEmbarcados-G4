# HydroWatch — protótipo mobile estático

Frontend demonstrativo feito apenas com HTML, CSS e JavaScript puro. A interface apresenta exclusivamente o nível da água medido pelo sensor ultrassônico, a distância até a superfície, os limites configurados e o histórico de leituras. Não há framework, etapa de build, banco de dados ou dependências externas.

## Executar localmente

Abra `dist/index.html` diretamente no navegador ou, para simular um servidor web:

```bash
cd dist
python3 -m http.server 8080
```

Depois acesse `http://localhost:8080`.

## Estrutura

- `dist/index.html`: estrutura e conteúdo das telas.
- `dist/styles.css`: identidade visual e responsividade.
- `dist/app.js`: navegação e interações com dados simulados.

## Integração futura

Os dados estão isolados em `app.js` no objeto `mockData`. Quando o Supabase estiver disponível, essa camada poderá ser substituída por consultas ao PostgreSQL usando o cliente oficial, sem redesenhar a interface.
