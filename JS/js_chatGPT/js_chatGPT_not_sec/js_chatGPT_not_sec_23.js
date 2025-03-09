import { createRouter, createWebHistory } from 'vue-router';

// Lazy-loaded components
const Home = () => import('./components/Home.vue');
const About = () => import('./components/About.vue');
const Dashboard = () => import('./components/Dashboard.vue');
const Profile = () => import('./components/Profile.vue');
const Settings = () => import('./components/Settings.vue');

// Route Guard function
function authGuard(to, from, next) {
    const isAuthenticated = Boolean(localStorage.getItem('auth')); // Example auth check
    if (isAuthenticated) {
        next();
    } else {
        next('/'); // Redirect to home if not authenticated
    }
}

// Routes definition
const routes = [
    {
        path: '/',
        name: 'Home',
        component: Home,
    },
    {
        path: '/about',
        name: 'About',
        component: About,
    },
    {
        path: '/dashboard',
        name: 'Dashboard',
        component: Dashboard,
        beforeEnter: authGuard, // Apply route guard
        children: [ // Nested routes
            {
                path: 'profile/:userId',
                name: 'Profile',
                component: Profile,
                props: route => ({ userId: route.params.userId }), // URL Parameter Mapping
            },
            {
                path: 'settings',
                name: 'Settings',
                component: Settings,
            },
        ],
    },
];

// Router instance creation
const router = createRouter({
    history: createWebHistory(),
    routes,
});

// Global Navigation Events
router.beforeEach((to, from, next) => {
    console.log(`Navigating from ${from.fullPath} to ${to.fullPath}`);
    next();
});

router.afterEach((to, from) => {
    console.log(`Navigation to ${to.fullPath} completed`);
});

export default router;
