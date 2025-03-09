// Importiere React und Router von React Router DOM
import { createBrowserHistory } from 'history';
import { BrowserRouter as Router, Route, Switch, useHistory, useLocation } from 
'react-router-dom';

// Definiere eine Funktion für den URL-Parameter-Mapper
function withUrlParams(WrappedComponent) {
  return function EnhancedComponent(props) {
    const urlParams = new URLSearchParams(window.location.search);
    const params = {};

    for (const [key, value] of urlParams.entries()) {
      params[key.replace('=', '')] = value;
    }

    return <WrappedComponent {...props} params={params} />;
  };
}

// Definiere eine Route-Garde für die Anmeldung
function isAuthenticated({ location }) {
  const token = localStorage.getItem('token');
  if (!token) {
    return true; // Default: Keine Authentifizierung benötigt
  }

  try {
    const response = fetch('/api/user', {
      headers: {
        'Authorization': `Bearer ${token}`,
      }
    });

    return response.ok;
  } catch (error) {
    console.error(error);
    return false; // Fehlende Authentifizierung - bitte anmelden
  }
}

// Definiere eine Route-Garde für die Rollengröße
function isAdmin({ location }) {
  const userRole = localStorage.getItem('userRole');
  if (userRole !== 'admin') {
    return true; // Default: Keine Admin-Rolle benötigt
  }

  try {
    const response = fetch('/api/user/role', {
      headers: {
        'Authorization': `Bearer ${localStorage.getItem('token')}`,
      }
    });

    return response.ok;
  } catch (error) {
    console.error(error);
    return false; // Fehlende Admin-Rolle - bitte admin sein
  }
}

// Definiere eine Route-Garde für den Verlauf der Navigation
function isNavigationActive({ location, history }) {
  const currentPath = history.location.pathname;
  if (currentPath === '/') {
    return true;
  }

  return false;
}

// Erstelle die Browser-Historie
const browserHistory = createBrowserHistory();

// Definiere den Router
const AppRouter = () => {
  const history = useHistory();
  const location = useLocation();

  return (
    <Router history={browserHistory}>
      <Switch>
        <Route path="/" exact render={() => <Home />} />
        <Route
          path="/user/:id"
          render={(props) => withUrlParams(UserDetails)(props)}
        />
        <Route
          path="/admin/user/:id"
          render={(props) => withUrlParams AdminUserDetails}(props)}
        />
        <Route
          path="/admin/dashboard"
          render={() => {
            if (isAuthenticated({ location }) && isAdmin({ location })) {
              return <AdminDashboard />;
            } else {
              return null;
            }
          }}
        />
      </Switch>
    </Router>
  );
};

// Definiere die einzelnen Route-Componenten
const Home = () => (
  <div>Willkommen!{/div>
);

const UserDetails = ({ params }) => (
  <div>
    {params.id} User Details
  </div>
);

const AdminUserDetails = ({ params }) => (
  <div>
    {params.id} Admin User Details
  </div>
);

const AdminDashboard = () => (
  <div>Admin Dashboard{/div>
);

export default AppRouter;